using System.Net;
using System.Text;
using System.Text.Json;
using ORIScore.ESPsim.Runner;

Console.OutputEncoding = Encoding.UTF8;

int port = ParsePort(args, 18088);
var state = new SimState();
var cts = new CancellationTokenSource();

Console.WriteLine($"SIM Runner starting on http://localhost:{port}/");

var httpTask = RunHttpServerAsync(port, state, cts.Token);
var clockTask = RunClockAsync(state, cts.Token);
var stdinTask = RunStdinAsync(state, cts);

await Task.WhenAny(httpTask, stdinTask);
cts.Cancel();

try { await Task.WhenAll(httpTask, clockTask, stdinTask); }
catch (OperationCanceledException) { }

static int ParsePort(string[] args, int fallback)
{
    for (int i = 0; i < args.Length - 1; i++)
    {
        if (args[i] == "--port" && int.TryParse(args[i + 1], out int p))
            return p;
    }

    return fallback;
}

static async Task RunClockAsync(SimState state, CancellationToken token)
{
    while (!token.IsCancellationRequested)
    {
        state.Tick();
        await Task.Delay(250, token);
    }
}

static async Task RunStdinAsync(SimState state, CancellationTokenSource cts)
{
    while (!cts.IsCancellationRequested)
    {
        string? line = await Console.In.ReadLineAsync();
        if (line == null)
        {
            await Task.Delay(50, cts.Token);
            continue;
        }

        line = line.Trim();
        if (line.Length == 0)
            continue;

        if (line.Equals("EXIT", StringComparison.OrdinalIgnoreCase))
        {
            cts.Cancel();
            break;
        }

        HandleCommand(state, line);
    }
}

static void HandleCommand(SimState state, string line)
{
    if (line.StartsWith("MODEM ", StringComparison.OrdinalIgnoreCase))
    {
        string payload = line[6..].Trim();
        state.InjectModemLine(payload);
        Console.WriteLine($"MODEM < {payload}");
        return;
    }

    if (line.StartsWith("SCENARIO ", StringComparison.OrdinalIgnoreCase))
    {
        string scenario = line[9..].Trim().ToUpperInvariant();
        state.RunScenario(scenario);
        Console.WriteLine($"SCENARIO {scenario}");
        return;
    }

    if (line.Equals("RESET", StringComparison.OrdinalIgnoreCase))
    {
        state.Reset();
        Console.WriteLine("SIM reset");
        return;
    }

    Console.WriteLine("WARN unknown command: " + line);
}

static async Task RunHttpServerAsync(int port, SimState state, CancellationToken token)
{
    using var listener = new HttpListener();
    listener.Prefixes.Add($"http://localhost:{port}/");
    listener.Start();

    Console.WriteLine($"HTTP listening on http://localhost:{port}/");

    while (!token.IsCancellationRequested)
    {
        HttpListenerContext ctx;
        try
        {
            ctx = await listener.GetContextAsync().WaitAsync(token);
        }
        catch (OperationCanceledException)
        {
            break;
        }

        _ = Task.Run(() => HandleHttpAsync(ctx, state), token);
    }
}

static async Task HandleHttpAsync(HttpListenerContext ctx, SimState state)
{
    try
    {
        string path = ctx.Request.Url?.AbsolutePath ?? "/";

        if (path.Equals("/api/status", StringComparison.OrdinalIgnoreCase))
        {
            await WriteJsonAsync(ctx, state.GetStatus());
            return;
        }

        if (path.Equals("/api/log", StringComparison.OrdinalIgnoreCase))
        {
            await WriteJsonAsync(ctx, state.GetLog());
            return;
        }

        if (path.Equals("/api/command", StringComparison.OrdinalIgnoreCase))
        {
            string action = ctx.Request.QueryString["action"] ?? "";
            state.WebAction(action);
            await WriteJsonAsync(ctx, new { ok = true, action });
            return;
        }

        await WriteHtmlAsync(ctx, HtmlPage());
    }
    catch (Exception ex)
    {
        ctx.Response.StatusCode = 500;
        await WriteTextAsync(ctx, "ERR " + ex.Message, "text/plain; charset=utf-8");
    }
}

static async Task WriteJsonAsync(HttpListenerContext ctx, object value)
{
    string json = JsonSerializer.Serialize(value, new JsonSerializerOptions { WriteIndented = true });
    await WriteTextAsync(ctx, json, "application/json; charset=utf-8");
}

static async Task WriteHtmlAsync(HttpListenerContext ctx, string html)
{
    await WriteTextAsync(ctx, html, "text/html; charset=utf-8");
}

static async Task WriteTextAsync(HttpListenerContext ctx, string text, string contentType)
{
    byte[] bytes = Encoding.UTF8.GetBytes(text);
    ctx.Response.ContentType = contentType;
    ctx.Response.ContentLength64 = bytes.Length;
    await ctx.Response.OutputStream.WriteAsync(bytes);
    ctx.Response.OutputStream.Close();
}

static string HtmlPage()
{
    return """
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ORIScore ESPsim - Device Web</title>
<style>
body{font-family:Arial,sans-serif;max-width:1100px;margin:20px auto;padding:0 12px;background:#f5f5f5;color:#111}
.card{background:#fff;border-radius:14px;padding:16px;margin:14px 0;box-shadow:0 2px 10px rgba(0,0,0,.08)}
.grid{display:grid;grid-template-columns:1fr 1fr 1fr;gap:12px}.pill{display:inline-block;padding:6px 10px;border-radius:999px;background:#eef2ff;margin:2px 4px 2px 0}.ok{background:#dcfce7}.bad{background:#fee2e2}.warn{background:#fef3c7}
button{padding:10px 14px;border-radius:10px;border:0;background:#6d28d9;color:#fff;cursor:pointer;margin:3px}pre{white-space:pre-wrap;background:#111827;color:#e5e7eb;padding:12px;border-radius:10px;max-height:260px;overflow:auto}
</style>
</head>
<body>
<h1>ORIScore ESPsim - fake web zařízení</h1>
<div class="card">
  <h2>Stav</h2>
  <div id="status">Načítám...</div>
</div>
<div class="card">
  <h2>Akce</h2>
  <button onclick="cmd('join')">Queue JOIN</button>
  <button onclick="cmd('send')">Queue SEND</button>
  <button onclick="cmd('fault_toggle')">Toggle porucha</button>
  <button onclick="cmd('manual_toggle')">Toggle manual</button>
</div>
<div class="card">
  <h2>Log</h2>
  <pre id="log"></pre>
</div>
<script>
async function load(){
  const s=await (await fetch('/api/status')).json();
  document.getElementById('status').innerHTML=`
  <div class="grid">
    <div><b>Uptime</b><br>${s.uptime}</div>
    <div><b>LoRa joined</b><br><span class="pill ${s.loraJoined?'ok':'bad'}">${s.loraJoined}</span></div>
    <div><b>Modem ready</b><br><span class="pill ${s.modemReady?'ok':'warn'}">${s.modemReady}</span></div>
    <div><b>UT ventil</b><br>${s.valveUT}%</div>
    <div><b>TV ventil</b><br>${s.valveTV}%</div>
    <div><b>Čerpadlo</b><br><span class="pill ${s.pumpUT?'ok':'bad'}">${s.pumpUT?'ON':'OFF'}</span></div>
    <div><b>Porucha</b><br><span class="pill ${s.fault?'bad':'ok'}">${s.fault?'AKTIVNÍ':'OK'}</span></div>
    <div><b>Last AT</b><br><code>${s.lastCommand}</code></div>
    <div><b>Last modem</b><br><code>${s.lastModem}</code></div>
  </div>`;
  const l=await (await fetch('/api/log')).json();
  document.getElementById('log').textContent=l.lines.join('\n');
}
async function cmd(a){ await fetch('/api/command?action='+encodeURIComponent(a)); await load(); }
setInterval(load,1000); load();
</script>
</body>
</html>
""";
}

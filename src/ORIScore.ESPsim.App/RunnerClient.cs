using System.Diagnostics;
using System.Text;

namespace ORIScore.ESPsim.App;

public sealed class RunnerClient : IDisposable
{
    private Process? _process;

    public event Action<string>? LineReceived;
    public event Action? Exited;

    public bool IsRunning => _process is { HasExited: false };

    public int Port { get; }

    public RunnerClient(int port = 18088)
    {
        Port = port;
    }

    public void Start()
    {
        if (IsRunning)
            return;

        string runnerPath = FindRunnerExecutable();
        if (!File.Exists(runnerPath))
            throw new FileNotFoundException("Runner executable not found. Build solution first.", runnerPath);

        var psi = new ProcessStartInfo
        {
            FileName = runnerPath,
            Arguments = $"--port {Port}",
            UseShellExecute = false,
            RedirectStandardInput = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = true,
            StandardOutputEncoding = Encoding.UTF8,
            StandardErrorEncoding = Encoding.UTF8,
            WorkingDirectory = Path.GetDirectoryName(runnerPath) ?? AppContext.BaseDirectory
        };

        _process = new Process { StartInfo = psi, EnableRaisingEvents = true };
        _process.OutputDataReceived += (_, e) => { if (e.Data != null) LineReceived?.Invoke(e.Data); };
        _process.ErrorDataReceived += (_, e) => { if (e.Data != null) LineReceived?.Invoke("ERR " + e.Data); };
        _process.Exited += (_, _) => Exited?.Invoke();

        _process.Start();
        _process.BeginOutputReadLine();
        _process.BeginErrorReadLine();
    }

    public void Send(string command)
    {
        if (!IsRunning || string.IsNullOrWhiteSpace(command))
            return;

        _process!.StandardInput.WriteLine(command);
        _process.StandardInput.Flush();
    }

    public void Stop()
    {
        if (!IsRunning)
            return;

        try
        {
            Send("EXIT");
            if (!_process!.WaitForExit(1000))
                _process.Kill(entireProcessTree: true);
        }
        catch
        {
            // Ignore shutdown errors.
        }
    }

    private static string FindRunnerExecutable()
    {
        string baseDir = AppContext.BaseDirectory;

        string portable = Path.Combine(AppPaths.RunnerDir, "ORIScore.ESPsim.Runner.exe");
        if (File.Exists(portable))
            return portable;

        string local = Path.Combine(baseDir, "ORIScore.ESPsim.Runner.exe");
        if (File.Exists(local))
            return local;

        var dir = new DirectoryInfo(baseDir);
        for (int i = 0; i < 8 && dir != null; i++, dir = dir.Parent)
        {
            string candidateDebug = Path.Combine(
                dir.FullName,
                "src",
                "ORIScore.ESPsim.Runner",
                "bin",
                "Debug",
                "net8.0",
                "ORIScore.ESPsim.Runner.exe"
            );

            if (File.Exists(candidateDebug))
                return candidateDebug;

            string candidateRelease = Path.Combine(
                dir.FullName,
                "src",
                "ORIScore.ESPsim.Runner",
                "bin",
                "Release",
                "net8.0",
                "ORIScore.ESPsim.Runner.exe"
            );

            if (File.Exists(candidateRelease))
                return candidateRelease;
        }

        return local;
    }

    public void Dispose()
    {
        Stop();
        _process?.Dispose();
    }
}

namespace ORIScore.ESPsim.Runner;

public sealed class SimState
{
    private readonly object _lock = new();
    private readonly List<string> _log = new();
    private readonly DateTime _started = DateTime.UtcNow;

    private string _lastCommand = "";
    private string _lastModem = "";
    private bool _modemReady;
    private bool _loraJoined;
    private bool _fault;
    private bool _manual;
    private bool _pumpUT;
    private int _valveUT;
    private int _valveTV;
    private int _tick;

    public SimState()
    {
        Log("SIM boot");
    }

    public void Reset()
    {
        lock (_lock)
        {
            _lastCommand = "";
            _lastModem = "";
            _modemReady = false;
            _loraJoined = false;
            _fault = false;
            _manual = false;
            _pumpUT = false;
            _valveUT = 0;
            _valveTV = 0;
            _tick = 0;
            _log.Clear();
            LogNoLock("SIM reset");
        }
    }

    public void Tick()
    {
        lock (_lock)
        {
            _tick++;
            if (_tick % 20 == 0)
            {
                _pumpUT = !_fault && !_manual;
            }
        }
    }

    public void InjectModemLine(string line)
    {
        lock (_lock)
        {
            _lastModem = line;

            if (line.Contains("OK", StringComparison.OrdinalIgnoreCase))
                _modemReady = true;

            if (line.Contains("JOINED", StringComparison.OrdinalIgnoreCase))
                _loraJoined = true;

            if (line.Contains("JOIN FAILED", StringComparison.OrdinalIgnoreCase) ||
                line.Contains("AT_NO_NETWORK_JOINED", StringComparison.OrdinalIgnoreCase))
                _loraJoined = false;

            LogNoLock("MODEM < " + line);
        }
    }

    public void RunScenario(string scenario)
    {
        lock (_lock)
        {
            switch (scenario)
            {
                case "JOIN_OK":
                    _lastCommand = "AT+JOIN";
                    _lastModem = "JOINED";
                    _modemReady = true;
                    _loraJoined = true;
                    LogNoLock("ESP > AT+JOIN");
                    LogNoLock("MODEM < JOINED");
                    break;

                case "SEND_OK":
                    _lastCommand = "AT+SEND=10:010203";
                    _lastModem = "DONE";
                    _modemReady = true;
                    LogNoLock("ESP > AT+SEND=10:010203");
                    LogNoLock("MODEM < DONE");
                    break;

                default:
                    LogNoLock("Unknown scenario: " + scenario);
                    break;
            }
        }
    }

    public void WebAction(string action)
    {
        lock (_lock)
        {
            switch (action)
            {
                case "join":
                    _lastCommand = "AT+JOIN";
                    LogNoLock("WEB queued JOIN");
                    LogNoLock("ESP > AT+JOIN");
                    break;

                case "send":
                    _lastCommand = "AT+SEND=10:FAKEPAYLOAD";
                    LogNoLock("WEB queued SEND");
                    LogNoLock("ESP > AT+SEND=10:FAKEPAYLOAD");
                    break;

                case "fault_toggle":
                    _fault = !_fault;
                    LogNoLock("WEB fault -> " + (_fault ? "ACTIVE" : "OK"));
                    break;

                case "manual_toggle":
                    _manual = !_manual;
                    LogNoLock("WEB manual -> " + (_manual ? "ON" : "OFF"));
                    break;

                default:
                    LogNoLock("WEB action: " + action);
                    break;
            }
        }
    }

    public object GetStatus()
    {
        lock (_lock)
        {
            return new
            {
                uptime = FormatUptime(DateTime.UtcNow - _started),
                modemReady = _modemReady,
                loraJoined = _loraJoined,
                fault = _fault,
                manual = _manual,
                pumpUT = _pumpUT,
                valveUT = _valveUT,
                valveTV = _valveTV,
                lastCommand = _lastCommand,
                lastModem = _lastModem
            };
        }
    }

    public object GetLog()
    {
        lock (_lock)
        {
            return new { lines = _log.ToArray() };
        }
    }

    private void Log(string text)
    {
        lock (_lock)
        {
            LogNoLock(text);
        }
    }

    private void LogNoLock(string text)
    {
        string line = $"[{DateTime.Now:HH:mm:ss}] {text}";
        _log.Add(line);
        if (_log.Count > 500)
            _log.RemoveRange(0, _log.Count - 500);

        Console.WriteLine(line);
    }

    private static string FormatUptime(TimeSpan t)
    {
        return $"{(int)t.TotalHours:00}:{t.Minutes:00}:{t.Seconds:00}";
    }
}

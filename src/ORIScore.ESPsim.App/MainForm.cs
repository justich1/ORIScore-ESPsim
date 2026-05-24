using System.Diagnostics;
using System.Globalization;
using System.Text.RegularExpressions;
using System.Text.Json;
using System.Text.Json.Serialization;
using Microsoft.Web.WebView2.WinForms;

namespace ORIScore.ESPsim.App;

public sealed class MainForm : Form
{
    private readonly BuildService _buildService = new();
    private readonly LibraryRepositoryService _libraryRepo = new();

    private Process? _firmwareProcess;
    private string? _lastFirmwareExe;

    private readonly WebView2 _webView = new();
    private readonly TextBox _log = new();
    private readonly TextBox _modemInput = new();
    private readonly TextBox _inoPath = new();
    private readonly TextBox _buildOutput = new();
    private readonly Label _status = new();
    private readonly FlowLayoutPanel _pinFlow = new();
    private readonly Dictionary<int, PinState> _pins = new();
    private readonly Dictionary<int, Button> _pinButtons = new();
    private readonly DataGridView _sensorGrid = new();
    private readonly ToolStripComboBox _speedCombo = new();
    private readonly ToolStripComboBox _boardCombo = new();
    private readonly List<BoardProfile> _boards = new();
    private BoardProfile? _selectedBoard;
    private GroupBox? _pinGroup;

    private readonly Panel _ramPanel = new();
    private readonly Label _ramInfoLabel = new();
    private int _ramTotalBytes = 81920;
    private int _ramFreeBytes = 81920;
    private int _ramMaxBlockBytes = 81920;
    private bool _ramRuntimeLive = false;

    private readonly Panel _displayPanel = new();
    private readonly ComboBox _displayControllerCombo = new();
    private readonly ComboBox _displayInterfaceCombo = new();
    private readonly ComboBox _displaySizeCombo = new();
    private readonly TextBox _displayAddressText = new();
    private readonly TextBox _displayPinsText = new();
    private readonly Label _displayInfoLabel = new();
    private int _displayWidth = 128;
    private int _displayHeight = 64;
    private byte[] _displayBuffer = Array.Empty<byte>();

    private const int DevicePort = 18088;
    private const string DeviceUrl = "http://localhost:18088/";

    private static readonly Regex PinStateRegex = new(
        @"PINSTATE\s+GPIO=(?<gpio>\d+)\s+MODE=(?<mode>-?\d+)\s+VALUE=(?<value>\d+)",
        RegexOptions.Compiled | RegexOptions.IgnoreCase
    );

    private static readonly Regex RamStateRegex = new(
        @"RAMSTATE\s+TOTAL=(?<total>\d+)\s+FREE=(?<free>\d+)\s+MAX=(?<max>\d+)",
        RegexOptions.Compiled | RegexOptions.IgnoreCase
    );

    public MainForm()
    {
        AppPaths.EnsurePortableLayout();

        Text = "ORIScore ESPsim";
        Width = 1500;
        Height = 900;
        MinimumSize = new Size(1150, 740);
        StartPosition = FormStartPosition.CenterScreen;

        LoadBoardProfiles();
        SelectDefaultBoard();
        BuildUi();
    }

    protected override async void OnShown(EventArgs e)
    {
        base.OnShown(e);

        try
        {
            await _webView.EnsureCoreWebView2Async();
            _webView.CoreWebView2.NavigateToString(StartPageHtml());
        }
        catch (Exception ex)
        {
            AppendLog("UI", "WebView2 init failed: " + ex.Message);
        }
    }

    protected override void OnFormClosing(FormClosingEventArgs e)
    {
        StopFirmware();
        base.OnFormClosing(e);
    }

    private void BuildUi()
    {
        var root = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            RowCount = 3,
            ColumnCount = 1
        };
        root.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        root.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        root.RowStyles.Add(new RowStyle(SizeType.AutoSize));

        var top = new ToolStrip { GripStyle = ToolStripGripStyle.Hidden };

        var btnOpenIno = new ToolStripButton("Otevřít INO");
        btnOpenIno.Click += (_, _) => OpenIno();

        var btnBuild = new ToolStripButton("Build INO");
        btnBuild.Click += async (_, _) => await BuildInoAsync();

        var btnRunFirmware = new ToolStripButton("Spustit firmware");
        btnRunFirmware.Click += (_, _) => RunFirmware();

        var btnStopFirmware = new ToolStripButton("Zastavit firmware");
        btnStopFirmware.Click += (_, _) => StopFirmware();

        var btnRestartFirmware = new ToolStripButton("Restart firmware");
        btnRestartFirmware.Click += (_, _) => RestartFirmware();

        var btnReload = new ToolStripButton("Reload web");
        btnReload.Click += (_, _) => NavigateDevice();

        var btnClear = new ToolStripButton("Smazat log");
        btnClear.Click += (_, _) =>
        {
            _log.Clear();
            _buildOutput.Clear();
        };

        var btnSyncRepo = new ToolStripButton("Sync repo");
        btnSyncRepo.ToolTipText = "Stáhne indexy repozitáře z https://www.oris-core.cz/repo-sim/";
        btnSyncRepo.Click += async (_, _) => await SyncRepositoryAsync();

        var btnLibraryManager = new ToolStripButton("Správce repo");
        btnLibraryManager.ToolTipText = "Otevře prohlížeč repozitáře: knihovny, desky a Virtual HW";
        btnLibraryManager.Click += (_, _) => OpenLibraryManager();

        _boardCombo.AutoSize = false;
        _boardCombo.Width = 220;
        _boardCombo.DropDownStyle = ComboBoxStyle.DropDownList;
        _boardCombo.ToolTipText = "Profil desky";

        _boardCombo.Items.Clear();
        foreach (var board in _boards)
            _boardCombo.Items.Add(board);

        if (_selectedBoard != null)
            _boardCombo.SelectedItem = _selectedBoard;
        else if (_boardCombo.Items.Count > 0)
            _boardCombo.SelectedIndex = 0;

        _boardCombo.Enabled = _boardCombo.Items.Count > 0;
        _boardCombo.SelectedIndexChanged += (_, _) =>
        {
            if (_boardCombo.SelectedItem is BoardProfile board)
            {
                ApplyBoardProfile(board);
                BuildPinButtons();
                ResetRamRuntimeView();

                if (_pinGroup != null)
                    _pinGroup.Text = $"{board.Name} GPIO panel - input piny lze přepínat kliknutím";

                AppendLog("BOARD", $"Selected: {board.Name} ({board.Id})");
            }
        };

        _speedCombo.AutoSize = false;
        _speedCombo.Width = 80;
        _speedCombo.DropDownStyle = ComboBoxStyle.DropDownList;
        _speedCombo.ToolTipText = "Rychlost simulace času";
        _speedCombo.Items.AddRange(new object[] { "1x", "5x", "10x", "20x", "50x" });
        _speedCombo.SelectedIndex = 0;
        _speedCombo.SelectedIndexChanged += (_, _) => SendTimeScaleToFirmware();

        top.Items.Add(btnOpenIno);
        top.Items.Add(btnBuild);
        top.Items.Add(btnRunFirmware);
        top.Items.Add(btnStopFirmware);
        top.Items.Add(btnRestartFirmware);
        top.Items.Add(new ToolStripSeparator());
        top.Items.Add(btnReload);
        top.Items.Add(new ToolStripSeparator());
        top.Items.Add(new ToolStripLabel("Rychlost:"));
        top.Items.Add(_speedCombo);
        top.Items.Add(new ToolStripSeparator());
        top.Items.Add(new ToolStripLabel("Deska:"));
        top.Items.Add(_boardCombo);
        top.Items.Add(new ToolStripSeparator());
        top.Items.Add(btnSyncRepo);
        top.Items.Add(btnLibraryManager);
        top.Items.Add(btnClear);

        var split = new SplitContainer
        {
            Dock = DockStyle.Fill,
            Orientation = Orientation.Vertical,
            SplitterDistance = 900
        };

        var leftTabs = new TabControl { Dock = DockStyle.Fill };
        var webTab = new TabPage("Web zařízení");
        var buildTab = new TabPage("Build output");
        var sensorsTab = new TabPage("Virtual HW");
        var displayTab = new TabPage("Displej");

        _webView.Dock = DockStyle.Fill;
        webTab.Controls.Add(_webView);

        _buildOutput.Dock = DockStyle.Fill;
        _buildOutput.Multiline = true;
        _buildOutput.ScrollBars = ScrollBars.Both;
        _buildOutput.ReadOnly = true;
        _buildOutput.WordWrap = false;
        _buildOutput.Font = new Font("Consolas", 9);
        _buildOutput.BackColor = Color.FromArgb(18, 18, 18);
        _buildOutput.ForeColor = Color.FromArgb(230, 230, 230);
        buildTab.Controls.Add(_buildOutput);

        BuildSensorsTab(sensorsTab);
        BuildDisplayTab(displayTab);

        leftTabs.TabPages.Add(webTab);
        leftTabs.TabPages.Add(buildTab);
        leftTabs.TabPages.Add(sensorsTab);
        leftTabs.TabPages.Add(displayTab);
        split.Panel1.Controls.Add(leftTabs);

        var right = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            RowCount = 6,
            ColumnCount = 1,
            Padding = new Padding(8)
        };
        right.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        right.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        right.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        right.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        right.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        right.RowStyles.Add(new RowStyle(SizeType.AutoSize));

        var title = new Label
        {
            Text = "Konzole / firmware log",
            Dock = DockStyle.Fill,
            Font = new Font(Font, FontStyle.Bold),
            Padding = new Padding(0, 0, 0, 6)
        };

        _inoPath.Dock = DockStyle.Top;
        _inoPath.ReadOnly = true;
        _inoPath.PlaceholderText = "Zatím není vybraný .ino soubor";

        _ramPanel.Dock = DockStyle.Top;
        _ramPanel.Height = 58;
        _ramPanel.MinimumSize = new Size(260, 58);
        _ramPanel.BackColor = Color.FromArgb(245, 245, 245);
        _ramPanel.Paint += RamPanel_Paint;

        _ramInfoLabel.Dock = DockStyle.Top;
        _ramInfoLabel.Height = 18;
        _ramInfoLabel.Font = new Font("Consolas", 8.5f);
        _ramInfoLabel.ForeColor = Color.FromArgb(50, 50, 50);
        _ramPanel.Controls.Add(_ramInfoLabel);
        ResetRamRuntimeView();

        _log.Dock = DockStyle.Fill;
        _log.Multiline = true;
        _log.ScrollBars = ScrollBars.Vertical;
        _log.ReadOnly = true;
        _log.Font = new Font("Consolas", 10);
        _log.BackColor = Color.FromArgb(20, 24, 32);
        _log.ForeColor = Color.FromArgb(230, 230, 230);

        var inputPanel = new TableLayoutPanel
        {
            Dock = DockStyle.Top,
            ColumnCount = 2,
            RowCount = 1,
            Height = 36
        };
        inputPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        inputPanel.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));

        _modemInput.Dock = DockStyle.Fill;
        _modemInput.PlaceholderText = "Odpověď modemu, např. JOINED nebo OK";
        _modemInput.KeyDown += (_, e) =>
        {
            if (e.KeyCode == Keys.Enter)
            {
                SendModemInput();
                e.SuppressKeyPress = true;
            }
        };

        var sendButton = new Button { Text = "Odeslat", Width = 95, Dock = DockStyle.Right };
        sendButton.Click += (_, _) => SendModemInput();

        inputPanel.Controls.Add(_modemInput, 0, 0);
        inputPanel.Controls.Add(sendButton, 1, 0);

        _status.Dock = DockStyle.Fill;
        _status.Text = "Firmware: neběží| Web: " + DeviceUrl;
        _status.ForeColor = Color.DarkRed;
        _status.Padding = new Padding(0, 6, 0, 0);

        right.Controls.Add(title, 0, 0);
        right.Controls.Add(_inoPath, 0, 1);
        right.Controls.Add(_ramPanel, 0, 2);
        right.Controls.Add(_log, 0, 3);
        right.Controls.Add(inputPanel, 0, 4);
        right.Controls.Add(_status, 0, 5);

        split.Panel2.Controls.Add(right);

        _pinGroup = new GroupBox
        {
            Text = $"{_selectedBoard?.Name ?? "Board"} GPIO panel - input piny lze přepínat kliknutím",
            Dock = DockStyle.Fill,
            Padding = new Padding(8),
            Height = 118
        };

        _pinFlow.Dock = DockStyle.Fill;
        _pinFlow.AutoScroll = true;
        _pinFlow.WrapContents = true;
        _pinGroup.Controls.Add(_pinFlow);
        BuildPinButtons();

        root.Controls.Add(top, 0, 0);
        root.Controls.Add(split, 0, 1);
        root.Controls.Add(_pinGroup, 0, 2);

        Controls.Add(root);
    }


    private void BuildDisplayTab(TabPage displayTab)
    {
        var root = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            RowCount = 2,
            ColumnCount = 1,
            Padding = new Padding(8)
        };
        root.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        root.RowStyles.Add(new RowStyle(SizeType.Percent, 100));

        var config = new GroupBox
        {
            Text = "Konfigurace displeje",
            Dock = DockStyle.Top,
            AutoSize = true,
            Padding = new Padding(8)
        };

        var cfg = new TableLayoutPanel
        {
            Dock = DockStyle.Top,
            AutoSize = true,
            ColumnCount = 6,
            RowCount = 2
        };
        cfg.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        cfg.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 150));
        cfg.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        cfg.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 150));
        cfg.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        cfg.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));

        _displayControllerCombo.DropDownStyle = ComboBoxStyle.DropDownList;
        _displayControllerCombo.Items.AddRange(new object[] { "SSD1306" });
        _displayControllerCombo.SelectedIndex = 0;
        _displayControllerCombo.Dock = DockStyle.Fill;

        _displayInterfaceCombo.DropDownStyle = ComboBoxStyle.DropDownList;
        _displayInterfaceCombo.Items.AddRange(new object[] { "I2C", "SPI" });
        _displayInterfaceCombo.SelectedIndex = 0;
        _displayInterfaceCombo.Dock = DockStyle.Fill;

        _displaySizeCombo.DropDownStyle = ComboBoxStyle.DropDownList;
        _displaySizeCombo.Items.AddRange(new object[] { "128x64", "128x32" });
        _displaySizeCombo.SelectedIndex = 0;
        _displaySizeCombo.Dock = DockStyle.Fill;

        _displayAddressText.Text = "0x3C";
        _displayAddressText.Dock = DockStyle.Fill;

        _displayPinsText.Text = "I2C: SDA/SCL, SPI: SCK/MOSI/CS/DC/RST";
        _displayPinsText.Dock = DockStyle.Fill;

        var btnApply = new Button { Text = "Použít pro náhled", Width = 130, Dock = DockStyle.Left };
        btnApply.Click += (_, _) => ApplyDisplayUiConfig();

        var btnClear = new Button { Text = "Vyčistit náhled", Width = 130, Dock = DockStyle.Left };
        btnClear.Click += (_, _) => ClearDisplayPreview();

        cfg.Controls.Add(new Label { Text = "Řadič:", AutoSize = true, TextAlign = ContentAlignment.MiddleLeft, Dock = DockStyle.Fill }, 0, 0);
        cfg.Controls.Add(_displayControllerCombo, 1, 0);
        cfg.Controls.Add(new Label { Text = "Rozhraní:", AutoSize = true, TextAlign = ContentAlignment.MiddleLeft, Dock = DockStyle.Fill }, 2, 0);
        cfg.Controls.Add(_displayInterfaceCombo, 3, 0);
        cfg.Controls.Add(new Label { Text = "Velikost:", AutoSize = true, TextAlign = ContentAlignment.MiddleLeft, Dock = DockStyle.Fill }, 4, 0);
        cfg.Controls.Add(_displaySizeCombo, 5, 0);

        cfg.Controls.Add(new Label { Text = "Adresa:", AutoSize = true, TextAlign = ContentAlignment.MiddleLeft, Dock = DockStyle.Fill }, 0, 1);
        cfg.Controls.Add(_displayAddressText, 1, 1);
        cfg.Controls.Add(new Label { Text = "Piny:", AutoSize = true, TextAlign = ContentAlignment.MiddleLeft, Dock = DockStyle.Fill }, 2, 1);
        cfg.Controls.Add(_displayPinsText, 3, 1);
        cfg.Controls.Add(btnApply, 4, 1);
        cfg.Controls.Add(btnClear, 5, 1);

        config.Controls.Add(cfg);

        var previewGroup = new GroupBox
        {
            Text = "Živý náhled z firmware",
            Dock = DockStyle.Fill,
            Padding = new Padding(8)
        };

        var previewRoot = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            RowCount = 2,
            ColumnCount = 1
        };
        previewRoot.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        previewRoot.RowStyles.Add(new RowStyle(SizeType.Percent, 100));

        _displayInfoLabel.Text = "SSD1306 I2C 128x64 — čekám na DISPLAYFRAME z U8glib";
        _displayInfoLabel.Dock = DockStyle.Top;
        _displayInfoLabel.Padding = new Padding(0, 0, 0, 6);

        _displayPanel.Dock = DockStyle.Fill;
        _displayPanel.MinimumSize = new Size(260, 160);
        _displayPanel.BackColor = Color.FromArgb(18, 18, 18);
        _displayPanel.Paint += DisplayPanel_Paint;

        previewRoot.Controls.Add(_displayInfoLabel, 0, 0);
        previewRoot.Controls.Add(_displayPanel, 0, 1);
        previewGroup.Controls.Add(previewRoot);

        root.Controls.Add(config, 0, 0);
        root.Controls.Add(previewGroup, 0, 1);
        displayTab.Controls.Add(root);

        ApplyDisplayUiConfig();
    }

    private void ApplyDisplayUiConfig()
    {
        string size = Convert.ToString(_displaySizeCombo.SelectedItem) ?? "128x64";
        if (size.Equals("128x32", StringComparison.OrdinalIgnoreCase))
        {
            _displayWidth = 128;
            _displayHeight = 32;
        }
        else
        {
            _displayWidth = 128;
            _displayHeight = 64;
        }

        int bytes = _displayWidth * ((_displayHeight + 7) / 8);
        if (_displayBuffer.Length != bytes)
            _displayBuffer = new byte[bytes];

        RefreshDisplayInfo("UI");
        _displayPanel.Invalidate();
    }

    private void ClearDisplayPreview()
    {
        Array.Clear(_displayBuffer, 0, _displayBuffer.Length);
        _displayPanel.Invalidate();
        AppendLog("DISPLAY", "Náhled vyčištěn");
    }

    private void RefreshDisplayInfo(string source)
    {
        string controller = Convert.ToString(_displayControllerCombo.SelectedItem) ?? "SSD1306";
        string iface = Convert.ToString(_displayInterfaceCombo.SelectedItem) ?? "I2C";
        _displayInfoLabel.Text = $"{controller} {iface} {_displayWidth}x{_displayHeight} [{source}]";
    }

    private void DisplayPanel_Paint(object? sender, PaintEventArgs e)
    {
        e.Graphics.Clear(Color.FromArgb(12, 14, 18));

        if (_displayWidth <= 0 || _displayHeight <= 0)
            return;

        float sx = _displayPanel.ClientSize.Width / (float)_displayWidth;
        float sy = _displayPanel.ClientSize.Height / (float)_displayHeight;
        float scale = Math.Max(1.0f, Math.Min(sx, sy));

        int drawW = (int)(_displayWidth * scale);
        int drawH = (int)(_displayHeight * scale);
        int offX = (_displayPanel.ClientSize.Width - drawW) / 2;
        int offY = (_displayPanel.ClientSize.Height - drawH) / 2;

        using var bg = new SolidBrush(Color.Black);
        using var on = new SolidBrush(Color.FromArgb(190, 235, 255));
        using var border = new Pen(Color.FromArgb(80, 80, 80));

        e.Graphics.FillRectangle(bg, offX, offY, drawW, drawH);
        e.Graphics.DrawRectangle(border, offX, offY, drawW - 1, drawH - 1);

        if (_displayBuffer.Length == 0)
            return;

        int pages = (_displayHeight + 7) / 8;

        for (int y = 0; y < _displayHeight; y++)
        {
            int page = y / 8;
            int bit = y & 7;

            for (int x = 0; x < _displayWidth; x++)
            {
                int index = page * _displayWidth + x;
                if (index < 0 || index >= _displayBuffer.Length)
                    continue;

                bool pixelOn = (_displayBuffer[index] & (1 << bit)) != 0;
                if (!pixelOn)
                    continue;

                int px = offX + (int)(x * scale);
                int py = offY + (int)(y * scale);
                int ps = Math.Max(1, (int)Math.Ceiling(scale));
                e.Graphics.FillRectangle(on, px, py, ps, ps);
            }
        }
    }

    private bool TryHandleDisplayRuntimeLine(string line)
    {
        if (line.StartsWith("DISPLAYCFG ", StringComparison.OrdinalIgnoreCase))
        {
            int w = ReadDisplayIntToken(line, "W", _displayWidth);
            int h = ReadDisplayIntToken(line, "H", _displayHeight);
            string controller = ReadDisplayTextToken(line, "TYPE", "SSD1306");
            string iface = ReadDisplayTextToken(line, "IF", "I2C");

            _displayWidth = Math.Clamp(w, 1, 512);
            _displayHeight = Math.Clamp(h, 1, 256);

            int bytes = _displayWidth * ((_displayHeight + 7) / 8);
            _displayBuffer = new byte[bytes];

            SetComboIfExists(_displayControllerCombo, controller);
            SetComboIfExists(_displayInterfaceCombo, iface);
            SetComboIfExists(_displaySizeCombo, $"{_displayWidth}x{_displayHeight}");

            RefreshDisplayInfo("firmware");
            _displayPanel.Invalidate();
            AppendLog("DISPLAY", $"Config {controller} {iface} {_displayWidth}x{_displayHeight}");
            return true;
        }

        if (line.StartsWith("DISPLAYFRAME ", StringComparison.OrdinalIgnoreCase))
        {
            int w = ReadDisplayIntToken(line, "W", _displayWidth);
            int h = ReadDisplayIntToken(line, "H", _displayHeight);
            string hex = ReadDisplayTextToken(line, "HEX", "");

            if (w <= 0 || h <= 0 || string.IsNullOrWhiteSpace(hex))
                return true;

            byte[] data = HexToBytes(hex);
            if (data.Length == 0)
                return true;

            _displayWidth = Math.Clamp(w, 1, 512);
            _displayHeight = Math.Clamp(h, 1, 256);
            _displayBuffer = data;
            RefreshDisplayInfo("firmware live");
            _displayPanel.Invalidate();
            return true;
        }

        return false;
    }

    private static void SetComboIfExists(ComboBox combo, string value)
    {
        for (int i = 0; i < combo.Items.Count; i++)
        {
            if (string.Equals(Convert.ToString(combo.Items[i]), value, StringComparison.OrdinalIgnoreCase))
            {
                combo.SelectedIndex = i;
                return;
            }
        }
    }

    private static int ReadDisplayIntToken(string line, string key, int fallback)
    {
        string text = ReadDisplayTextToken(line, key, "");
        return int.TryParse(text, NumberStyles.Integer, CultureInfo.InvariantCulture, out int value)
            ? value
            : fallback;
    }

    private static string ReadDisplayTextToken(string line, string key, string fallback)
    {
        string prefix = key + "=";
        foreach (string part in line.Split(' ', StringSplitOptions.RemoveEmptyEntries))
        {
            if (part.StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
                return part[prefix.Length..];
        }

        return fallback;
    }

    private static byte[] HexToBytes(string hex)
    {
        hex = hex.Trim();
        if (hex.Length < 2)
            return Array.Empty<byte>();

        if ((hex.Length & 1) != 0)
            hex = hex[..^1];

        byte[] data = new byte[hex.Length / 2];

        for (int i = 0; i < data.Length; i++)
        {
            string b = hex.Substring(i * 2, 2);
            if (!byte.TryParse(b, NumberStyles.HexNumber, CultureInfo.InvariantCulture, out data[i]))
                return Array.Empty<byte>();
        }

        return data;
    }


    private static readonly string[] VirtualHwTypes =
    {
        "DS18B20",
        "DHT11",
        "DHT22"
    };

    private void BuildSensorsTab(TabPage sensorsTab)
    {
        var root = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            RowCount = 2,
            ColumnCount = 1,
            Padding = new Padding(8)
        };
        root.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        root.RowStyles.Add(new RowStyle(SizeType.AutoSize));

        _sensorGrid.Dock = DockStyle.Fill;
        _sensorGrid.AllowUserToAddRows = true;
        _sensorGrid.AllowUserToDeleteRows = true;
        _sensorGrid.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.Fill;
        _sensorGrid.RowHeadersWidth = 28;
        _sensorGrid.DataError += (_, e) =>
        {
            // Repo může obsahovat nový typ HW dřív, než ho zná UI.
            // Nechceme kvůli tomu shodit DataGridView.
            e.ThrowException = false;
            e.Cancel = true;
        };

        _sensorGrid.Columns.Add(new DataGridViewTextBoxColumn
        {
            Name = "Type",
            HeaderText = "Typ HW",
            FillWeight = 120
        });

        _sensorGrid.Columns.Add(new DataGridViewTextBoxColumn
        {
            Name = "Name",
            HeaderText = "Název",
            FillWeight = 120
        });

        _sensorGrid.Columns.Add(new DataGridViewTextBoxColumn
        {
            Name = "Pin",
            HeaderText = "Pin / bus",
            FillWeight = 85
        });

        _sensorGrid.Columns.Add(new DataGridViewTextBoxColumn
        {
            Name = "Address",
            HeaderText = "Adresa / ID",
            FillWeight = 135
        });

        _sensorGrid.Columns.Add(new DataGridViewTextBoxColumn
        {
            Name = "Value",
            HeaderText = "Hodnota / teplota / napětí / frekvence",
            FillWeight = 120
        });

        _sensorGrid.Columns.Add(new DataGridViewTextBoxColumn
        {
            Name = "Humidity",
            HeaderText = "Vlhkost / proud / pulzy/l",
            FillWeight = 110
        });

        _sensorGrid.Columns.Add(new DataGridViewTextBoxColumn
        {
            Name = "Pressure",
            HeaderText = "Tlak / výkon",
            FillWeight = 90
        });

        _sensorGrid.Columns.Add(new DataGridViewCheckBoxColumn
        {
            Name = "Connected",
            HeaderText = "Připojeno",
            FillWeight = 65
        });

        var buttons = new FlowLayoutPanel
        {
            Dock = DockStyle.Fill,
            AutoSize = true,
            FlowDirection = FlowDirection.LeftToRight
        };

        var btnDefaults = new Button { Text = "Vložit default HW", Width = 150 };
        btnDefaults.Click += (_, _) => AddDefaultSensors();

        var btnApply = new Button { Text = "Poslat do simulace", Width = 150 };
        btnApply.Click += (_, _) => ApplySensorsToFirmware();

        var btnLoad = new Button { Text = "Načíst JSON", Width = 120 };
        btnLoad.Click += (_, _) => LoadVirtualHardwareJson();

        var btnSave = new Button { Text = "Uložit JSON", Width = 120 };
        btnSave.Click += (_, _) => SaveVirtualHardwareJson();

        var btnRemove = new Button { Text = "Smazat vybraný řádek", Width = 160 };
        btnRemove.Click += (_, _) =>
        {
            foreach (DataGridViewRow row in _sensorGrid.SelectedRows)
            {
                if (!row.IsNewRow)
                    _sensorGrid.Rows.Remove(row);
            }
        };

        var btnClear = new Button { Text = "Smazat všechen HW", Width = 120 };
        btnClear.Click += (_, _) =>
        {
            _sensorGrid.Rows.Clear();
        };

        buttons.Controls.Add(btnDefaults);
        buttons.Controls.Add(btnApply);
        buttons.Controls.Add(btnLoad);
        buttons.Controls.Add(btnSave);
        buttons.Controls.Add(btnRemove);
        buttons.Controls.Add(btnClear);

        root.Controls.Add(_sensorGrid, 0, 0);
        root.Controls.Add(buttons, 0, 1);
        sensorsTab.Controls.Add(root);

        AddDefaultSensors();
    }

    private void AddDefaultSensors()
    {
        if (_sensorGrid.Rows.Cast<DataGridViewRow>().Any(r => !r.IsNewRow))
            return;
    }

    private void AddSensorRow(string type, string name, string pin, string address, string value, string humidity, string pressure, bool connected)
    {
        int row = _sensorGrid.Rows.Add();

        _sensorGrid.Rows[row].Cells["Type"].Value = type;
        _sensorGrid.Rows[row].Cells["Name"].Value = name;
        _sensorGrid.Rows[row].Cells["Pin"].Value = pin;
        _sensorGrid.Rows[row].Cells["Address"].Value = address;
        _sensorGrid.Rows[row].Cells["Value"].Value = value;
        _sensorGrid.Rows[row].Cells["Humidity"].Value = humidity;
        _sensorGrid.Rows[row].Cells["Pressure"].Value = pressure;
        _sensorGrid.Rows[row].Cells["Connected"].Value = connected;
    }

    private static string CellText(DataGridViewRow row, string name, string fallback = "")
    {
        return Convert.ToString(row.Cells[name].Value)?.Trim() ?? fallback;
    }

    private static bool CellBool(DataGridViewRow row, string name, bool fallback = true)
    {
        object? v = row.Cells[name].Value;
        if (v == null) return fallback;
        try { return Convert.ToBoolean(v); }
        catch { return fallback; }
    }

    private static string NormalizeDsAddress(string address)
    {
        // DS18B20 může být zadané jako:
        //  - plná Dallas adresa: 28FF641D64160328 = 16 HEX znaků
        //  - Linux / w1 formát bez CRC: 28-000000000001 = 14 HEX znaků
        // Pro simulaci doplníme chybějící CRC bajt jako 00, aby z toho byla DeviceAddress[8].
        string s = Regex.Replace((address ?? string.Empty).Trim().ToUpperInvariant(), "[^0-9A-F]", "");

        if (s.Length == 16)
            return s;

        if (s.Length == 14 && s.StartsWith("28", StringComparison.OrdinalIgnoreCase))
            return s + "00";

        return "";
    }

    private static string NormalizeGpioNumber(string pin)
    {
        string s = pin.Trim().ToUpperInvariant();
        if (s.StartsWith("GPIO", StringComparison.OrdinalIgnoreCase))
            s = s[4..];
        return s;
    }

    private void ApplySensorsToFirmware()
    {
        if (_firmwareProcess is not { HasExited: false })
        {
            AppendLog("VHW", "Neběží firmware, virtuální HW se pošle až po Run firmware + Apply.");
            return;
        }

        SendFirmwareCommand("HW CLEAR");

        int sent = 0;

        foreach (DataGridViewRow row in _sensorGrid.Rows)
        {
            if (row.IsNewRow) continue;

            string type = CellText(row, "Type").ToUpperInvariant();
            string name = CellText(row, "Name");
            string pin = CellText(row, "Pin");
            string address = CellText(row, "Address");
            string value = CellText(row, "Value").Replace(',', '.');
            string humidity = CellText(row, "Humidity").Replace(',', '.');
            string pressure = CellText(row, "Pressure").Replace(',', '.');
            bool connected = CellBool(row, "Connected", true);

            if (string.IsNullOrWhiteSpace(type))
                continue;

            if (type == "DS18B20")
            {
                address = NormalizeDsAddress(address);
                if (string.IsNullOrWhiteSpace(address))
                {
                    AppendLog("VHW", $"Ignoruji DS18B20 s neplatnou adresou: {CellText(row, "Address")}");
                    continue;
                }
            }

            SendFirmwareCommand("HW SET|" + EscapeHw(type) + "|" + EscapeHw(name) + "|" + EscapeHw(pin) + "|" +
                               EscapeHw(address) + "|" + EscapeHw(value) + "|" + EscapeHw(humidity) + "|" +
                               EscapeHw(pressure) + "|" + (connected ? "1" : "0"));
            sent++;
        }

        AppendLog("VHW", $"Odesláno do simulace: {sent} zařízení");
    }

    private static string EscapeHw(string value)
    {
        return (value ?? string.Empty)
            .Replace("\\", "\\\\")
            .Replace("|", "\\p")
            .Replace("\r", " ")
            .Replace("\n", " ");
    }

    private void SaveVirtualHardwareJson()
    {
        AppPaths.EnsurePortableLayout();
        using var dlg = new SaveFileDialog
        {
            Title = "Uložit Virtual HW JSON",
            Filter = "Virtual HW JSON (*.json)|*.json|Všechny soubory (*.*)|*.*",
            InitialDirectory = AppPaths.DevicesDir,
            FileName = "project_devices.json"
        };

        if (dlg.ShowDialog(this) != DialogResult.OK)
            return;

        var devices = new List<Dictionary<string, object?>>();
        foreach (DataGridViewRow row in _sensorGrid.Rows)
        {
            if (row.IsNewRow) continue;

            var d = new Dictionary<string, object?>
            {
                ["type"] = CellText(row, "Type", "DS18B20"),
                ["name"] = CellText(row, "Name"),
                ["pin"] = CellText(row, "Pin", "GPIO4"),
                ["address"] = CellText(row, "Address"),
                ["value"] = CellText(row, "Value"),
                ["humidity"] = CellText(row, "Humidity"),
                ["pressure"] = CellText(row, "Pressure"),
                ["connected"] = CellBool(row, "Connected", true)
            };

            // Zpětná kompatibilita pro staré DS18B20 JSONy.
            if (string.Equals(Convert.ToString(d["type"]), "DS18B20", StringComparison.OrdinalIgnoreCase))
                d["temperature"] = d["value"];

            devices.Add(d);
        }

        var root = new Dictionary<string, object?>
        {
            ["schema"] = "oriscore.espsim.devices.v2",
            ["devices"] = devices
        };

        File.WriteAllText(dlg.FileName, JsonSerializer.Serialize(root, new JsonSerializerOptions { WriteIndented = true }));
        AppendLog("VHW", "Uloženo: " + dlg.FileName);
    }

    private void LoadVirtualHardwareJson()
    {
        using var dlg = new OpenFileDialog
        {
            Title = "Načíst Virtual HW JSON",
            Filter = "Virtual HW JSON (*.json)|*.json|Všechny soubory (*.*)|*.*",
            InitialDirectory = Directory.Exists(AppPaths.DevicesDir) ? AppPaths.DevicesDir : AppContext.BaseDirectory
        };

        if (dlg.ShowDialog(this) != DialogResult.OK)
            return;

        try
        {
            LoadVirtualHardwareJsonFile(dlg.FileName);
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, ex.Message, "Virtual HW JSON", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    private void LoadVirtualHardwareJsonFile(string fileName)
    {
        using var doc = JsonDocument.Parse(File.ReadAllText(fileName));
        if (!doc.RootElement.TryGetProperty("devices", out var arr) || arr.ValueKind != JsonValueKind.Array)
            throw new InvalidOperationException("JSON neobsahuje pole devices.");

        foreach (var d in arr.EnumerateArray())
        {
            string type = JsonString(d, "type", "DS18B20");
            string name = JsonString(d, "name", "");
            string pin = JsonString(d, "pin", JsonString(d, "data", JsonString(d, "uart", "")));

            if (string.IsNullOrWhiteSpace(pin))
            {
                // I2C JSONy mívají sda/scl/address místo jednoho pinu.
                // Pro Virtual HW stačí symbolický pin I2C, knihovny potom párují podle address.
                if (d.TryGetProperty("sda", out _) || d.TryGetProperty("scl", out _))
                    pin = "I2C";
                else
                    pin = JsonString(d, "address", "GPIO4");
            }

            string address = JsonString(d, "address", JsonString(d, "id", ""));

            string value =
                JsonValueAsText(d, "temperature",
                    JsonValueAsText(d, "value",
                        JsonValueAsText(d, "ppm",
                            JsonValueAsText(d, "busVoltage",
                                JsonValueAsText(d, "state",
                                    JsonValueAsText(d, "frequency", ""))))));

            string humidity = JsonValueAsText(d, "humidity",
                JsonValueAsText(d, "current",
                    JsonValueAsText(d, "pulsesPerLiter", "")));

            string pressure = JsonValueAsText(d, "pressure",
                JsonValueAsText(d, "power", ""));

            bool connected = JsonBool(d, "connected", true);

            AddSensorRow(type, name, pin, address, value, humidity, pressure, connected);
        }

        AppendLog("VHW", "Načteno: " + fileName);
    }

    private static string JsonString(JsonElement e, string prop, string fallback)
    {
        return e.TryGetProperty(prop, out var v) && v.ValueKind == JsonValueKind.String
            ? v.GetString() ?? fallback
            : fallback;
    }

    private static bool JsonBool(JsonElement e, string prop, bool fallback)
    {
        if (!e.TryGetProperty(prop, out var v)) return fallback;
        if (v.ValueKind == JsonValueKind.True) return true;
        if (v.ValueKind == JsonValueKind.False) return false;
        if (v.ValueKind == JsonValueKind.String && bool.TryParse(v.GetString(), out bool b)) return b;
        return fallback;
    }

    private static string JsonValueAsText(JsonElement e, string prop, string fallback)
    {
        if (!e.TryGetProperty(prop, out var v)) return fallback;
        return v.ValueKind switch
        {
            JsonValueKind.Number => v.GetDouble().ToString(CultureInfo.InvariantCulture),
            JsonValueKind.String => v.GetString() ?? fallback,
            JsonValueKind.True => "1",
            JsonValueKind.False => "0",
            _ => fallback
        };
    }

    private void LoadBoardProfiles()
    {
        _boards.Clear();

        string boardsDir = FindBoardsDirectory();

        if (Directory.Exists(boardsDir))
        {
            foreach (string file in Directory.GetFiles(boardsDir, "*.json").OrderBy(x => x))
            {
                try
                {
                    string json = File.ReadAllText(file);
                    var board = JsonSerializer.Deserialize<BoardProfile>(
                        json,
                        new JsonSerializerOptions
                        {
                            PropertyNameCaseInsensitive = true,
                            ReadCommentHandling = JsonCommentHandling.Skip,
                            AllowTrailingCommas = true
                        }
                    );

                    if (board == null)
                        continue;

                    if (string.IsNullOrWhiteSpace(board.Id))
                        board.Id = Path.GetFileNameWithoutExtension(file);

                    if (string.IsNullOrWhiteSpace(board.Name))
                        board.Name = board.Id;

                    board.SourceFile = file;

                    if (board.Pins.Count > 0)
                        _boards.Add(board);
                }
                catch (Exception ex)
                {
                    AppendLog("BOARD", $"Nelze načíst {Path.GetFileName(file)}: {ex.Message}");
                }
            }
        }

        if (_boards.Count == 0)
            _boards.Add(CreateFallbackEsp8266Board());
    }

    private void SelectDefaultBoard()
    {
        _selectedBoard =
            _boards.FirstOrDefault(b => b.Id.Equals("esp8266_nodemcu_v3", StringComparison.OrdinalIgnoreCase))
            ?? _boards.FirstOrDefault();

        if (_selectedBoard != null)
            ApplyBoardProfile(_selectedBoard);
    }

    private void ApplyBoardProfile(BoardProfile board)
    {
        _selectedBoard = board;
        _pins.Clear();

        foreach (var p in board.Pins.OrderBy(p => PinSortOrder(p.BoardPin)))
        {
            if (p.Gpio < 0)
                continue;

            string boardPin = string.IsNullOrWhiteSpace(p.BoardPin)
                ? "GPIO" + p.Gpio
                : p.BoardPin;

            string gpioLabel = string.IsNullOrWhiteSpace(p.GpioLabel)
                ? "GPIO" + p.Gpio
                : p.GpioLabel;

            string role = p.Role ?? "";

            var state = new PinState(p.Gpio, boardPin, gpioLabel, role, p.ActiveLow)
            {
                Value = p.DefaultValue ?? 1,
                Mode = p.DefaultMode switch
                {
                    "INPUT" => 0,
                    "OUTPUT" => 1,
                    "INPUT_PULLUP" => 2,
                    _ => -1
                }
            };

            _pins[p.Gpio] = state;
        }

        _ramTotalBytes = GetBoardRamBytes(board);
        _ramFreeBytes = Math.Min(_ramFreeBytes, _ramTotalBytes);
        _ramMaxBlockBytes = Math.Min(_ramMaxBlockBytes, _ramFreeBytes);
    }

    private static BoardProfile CreateFallbackEsp8266Board()
    {
        return new BoardProfile
        {
            Id = "esp8266_nodemcu_v3",
            Name = "ESP8266 NodeMCU v3",
            RamBytes = 81920,
            Pins =
            {
                new BoardPinProfile { BoardPin = "D0", Gpio = 16, Role = "GPIO16 / WAKE", ActiveLow = true },
                new BoardPinProfile { BoardPin = "D1", Gpio = 5,  Role = "GPIO5 / I2C SCL",     ActiveLow = true },
                new BoardPinProfile { BoardPin = "D2", Gpio = 4,  Role = "GPIO4 / I2C SDA",    ActiveLow = true },
                new BoardPinProfile { BoardPin = "D3", Gpio = 0,  Role = "GPIO0 / BOOT INPUT",    ActiveLow = true, DefaultMode = "INPUT_PULLUP" },
                new BoardPinProfile { BoardPin = "D4", Gpio = 2,  Role = "GPIO2 / ONBOARD LED / ONEWIRE",     ActiveLow = false },
                new BoardPinProfile { BoardPin = "D5", Gpio = 14, Role = "GPIO14 / SPI SCLK",     ActiveLow = true },
                new BoardPinProfile { BoardPin = "D6", Gpio = 12, Role = "GPIO12 / SPI MISO",    ActiveLow = true },
                new BoardPinProfile { BoardPin = "D7", Gpio = 13, Role = "GPIO13 / SPI MOSI",     ActiveLow = true },
                new BoardPinProfile { BoardPin = "D8", Gpio = 15, Role = "GPIO15 / SPI CS / BOOT",       ActiveLow = true }
            }
        };
    }

    private static string FindBoardsDirectory()
    {
        return AppPaths.FindDirectory(
            AppPaths.BoardsDir,
            "boards",
            "Boards"
        );
    }

    private void BuildPinButtons()
    {
        _pinFlow.Controls.Clear();
        _pinButtons.Clear();

        foreach (var pin in _pins.Values.OrderBy(p => PinSortOrder(p.BoardPin)))
        {
            var button = new Button
            {
                Width = 150,
                Height = 58,
                Margin = new Padding(3),
                Font = new Font("Consolas", 8.5f),
                TextAlign = ContentAlignment.MiddleLeft,
                Tag = pin.Gpio
            };
            button.Click += (_, _) => ToggleInputPin((int)button.Tag!);
            _pinButtons[pin.Gpio] = button;
            _pinFlow.Controls.Add(button);
        }

        RefreshAllPins();
    }

    private static int PinSortOrder(string boardPin)
    {
        if (boardPin.StartsWith("D") && int.TryParse(boardPin[1..], out int n)) return n;
        return 100;
    }

    private void ToggleInputPin(int gpio)
    {
        if (!_pins.TryGetValue(gpio, out var pin)) return;

        if (pin.Mode is not (0 or 2))
        {
            AppendLog("GPIO", $"{pin.BoardPin}/{pin.GpioLabel} není INPUT, klik ignorován");
            return;
        }

        int newValue = pin.Value == 0 ? 1 : 0;
        SendFirmwareCommand($"PIN {gpio} {(newValue == 1 ? "HIGH" : "LOW")}");
    }

    private void UpdatePinState(int gpio, int mode, int value)
    {
        if (!_pins.TryGetValue(gpio, out var pin))
        {
            pin = new PinState(gpio, "GPIO" + gpio, "GPIO" + gpio, "");
            _pins[gpio] = pin;
        }

        pin.Mode = mode;
        pin.Value = value;
        RefreshPin(pin);
    }

    private void RefreshAllPins()
    {
        foreach (var pin in _pins.Values)
            RefreshPin(pin);
    }

    private void RefreshPin(PinState pin)
    {
        if (!_pinButtons.TryGetValue(pin.Gpio, out var button)) return;

        string mode = pin.Mode switch
        {
            0 => "INPUT",
            1 => "OUTPUT",
            2 => "INPUT_PULLUP",
            _ => "UNKNOWN"
        };

        bool rawHigh = pin.Value != 0;
        bool active = pin.ActiveLow ? !rawHigh : rawHigh;

        string rawValue = rawHigh ? "HIGH" : "LOW";
        string logic = active ? "ACTIVE" : "OFF";
        string click = pin.Mode is 0 or 2 ? "click" : "";
        string inv = pin.ActiveLow ? "INV" : "NOR";

        button.Text =
            $"{pin.BoardPin} {pin.GpioLabel} {inv}\r\n" +
            $"{pin.Role}\r\n" +
            $"{mode} raw={rawValue} logic={logic} {click}";

        button.Enabled = true;

        if (pin.Mode == 1)
        {
            button.BackColor = active
                ? Color.FromArgb(220, 255, 220)
                : Color.FromArgb(255, 224, 224);
            button.ForeColor = Color.Black;
        }
        else if (pin.Mode is 0 or 2)
        {
            button.BackColor = active
                ? Color.FromArgb(255, 210, 210)
                : Color.FromArgb(220, 255, 220);
            button.ForeColor = Color.Black;
        }
        else
        {
            button.BackColor = Color.FromArgb(235, 235, 235);
            button.ForeColor = Color.DimGray;
        }
    }

    private void OpenLibraryManager()
    {
        using var form = new LibraryManagerForm(_libraryRepo);
        form.ShowDialog(this);
    }

    private async Task SyncRepositoryAsync()
    {
        AppendLog("REPO", "Synchronizuji knihovní repo: " + AppPaths.DefaultRepositoryUrl);

        try
        {
            var result = await _libraryRepo.SyncAsync();
            AppendLog("REPO", result.Message);
            AppendLog("REPO", "Uloženo do: " + result.TargetDirectory);

            if (!result.Success)
                MessageBox.Show(this, result.Message, "Repo sync", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }
        catch (Exception ex)
        {
            AppendLog("REPO", "ERROR: " + ex.Message);
            MessageBox.Show(this, ex.Message, "Repo sync", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    private void OpenIno()
    {
        using var dialog = new OpenFileDialog
        {
            Filter = "Arduino sketch (*.ino)|*.ino|C++ source (*.cpp)|*.cpp|All files (*.*)|*.*",
            Title = "Vyber INO soubor"
        };

        if (dialog.ShowDialog(this) != DialogResult.OK)
            return;

        _inoPath.Text = dialog.FileName;
        AppendLog("APP", "INO selected: " + dialog.FileName);
    }

    private async Task BuildInoAsync()
    {
        string path = _inoPath.Text.Trim();
        if (path.Length == 0 || !File.Exists(path))
        {
            MessageBox.Show(this, "Nejdřív vyber .ino soubor.", "Build", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }

        StopFirmware();
        _buildOutput.Clear();
        AppendLog("BUILD", "Build started");

        try
        {
            var result = await _buildService.BuildInoAsync(path);
            _buildOutput.Text = result.Output.Replace("\n", Environment.NewLine);

            if (result.Success && result.ExePath != null)
            {
                _lastFirmwareExe = result.ExePath;
                AppendLog("BUILD", "OK: " + result.ExePath);
            }
            else
            {
                _lastFirmwareExe = null;
                AppendLog("BUILD", "FAILED");
            }
        }
        catch (Exception ex)
        {
            _lastFirmwareExe = null;
            _buildOutput.Text = ex.ToString();
            AppendLog("BUILD", "ERROR: " + ex.Message);
        }

        UpdateStatus();
    }

    private void RunFirmware()
    {
        if (_firmwareProcess is { HasExited: false })
            return;

        if (string.IsNullOrWhiteSpace(_lastFirmwareExe) || !File.Exists(_lastFirmwareExe))
        {
            MessageBox.Show(this, "Nejdřív úspěšně zkompiluj .ino.", "Run firmware", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }

        ResetPinRuntimeView();
        ResetRamRuntimeView();

        var psi = new ProcessStartInfo
        {
            FileName = _lastFirmwareExe,
            WorkingDirectory = Path.GetDirectoryName(_lastFirmwareExe) ?? AppContext.BaseDirectory,
            UseShellExecute = false,
            RedirectStandardInput = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = true
        };
        psi.Environment["ORISIM_HTTP_PORT"] = DevicePort.ToString();
        psi.Environment["ORISIM_TIME_SCALE"] = GetSelectedTimeScale().ToString(CultureInfo.InvariantCulture);
        psi.Environment["ORISIM_FS_ROOT"] = GetLittleFsRoot();
        psi.Environment["ORISIM_RAM_TOTAL"] = GetBoardRamBytes(_selectedBoard).ToString(CultureInfo.InvariantCulture);

        _firmwareProcess = new Process { StartInfo = psi, EnableRaisingEvents = true };
        _firmwareProcess.OutputDataReceived += (_, e) =>
        {
            if (e.Data != null && !IsDisposed)
                BeginInvoke(() => HandleRuntimeLine(e.Data));
        };
        _firmwareProcess.ErrorDataReceived += (_, e) =>
        {
            if (e.Data != null && !IsDisposed)
                BeginInvoke(() => HandleRuntimeLine("ERR " + e.Data));
        };
        _firmwareProcess.Exited += (sender, _) =>
        {
            int? exitCode = null;

            try
            {
                if (sender is Process p)
                    exitCode = p.ExitCode;
            }
            catch
            {
                // ignore
            }

            if (!IsDisposed)
            {
                BeginInvoke(() =>
                {
                    AppendLog("FW", "Firmware exited, code=" + (exitCode?.ToString() ?? "?"));
                    UpdateStatus();
                });
            }
        };

        _firmwareProcess.Start();
        _firmwareProcess.BeginOutputReadLine();
        _firmwareProcess.BeginErrorReadLine();

        AppendLog("FW", "Firmware started");
        UpdateStatus();

        _ = Task.Delay(150).ContinueWith(_ =>
        {
            if (!IsDisposed)
                BeginInvoke(() => ApplySensorsToFirmware());
        });
    }


    private void RestartFirmware()
    {
        if (string.IsNullOrWhiteSpace(_lastFirmwareExe) || !File.Exists(_lastFirmwareExe))
        {
            MessageBox.Show(this, "Nejdřív úspěšně zkompiluj .ino.", "Restart firmware", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }

        AppendLog("FW", "Restart requested");
        StopFirmware();
        RunFirmware();
    }

    private double GetSelectedTimeScale()
    {
        string text = Convert.ToString(_speedCombo.SelectedItem) ?? "1x";
        text = text.Trim().TrimEnd('x', 'X').Replace(',', '.');
        return double.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out double v) && v > 0
            ? v
            : 1.0;
    }

    private void SendTimeScaleToFirmware()
    {
        if (_firmwareProcess is not { HasExited: false })
            return;

        SendFirmwareCommand("TIMESCALE " + GetSelectedTimeScale().ToString(CultureInfo.InvariantCulture));
    }

    private string GetLittleFsRoot()
    {
        string baseDir = Path.Combine(
            AppPaths.DeviceFsDir,
            _selectedBoard?.Id ?? "esp8266_nodemcu_v3"
        );

        Directory.CreateDirectory(baseDir);
        return baseDir;
    }

    private void StopFirmware()
    {
        if (_firmwareProcess is not { HasExited: false })
            return;

        try
        {
            _firmwareProcess.StandardInput.WriteLine("EXIT");
            _firmwareProcess.StandardInput.Flush();
            if (!_firmwareProcess.WaitForExit(1000))
                _firmwareProcess.Kill(entireProcessTree: true);
        }
        catch
        {
            // ignore
        }

        AppendLog("FW", "Stop requested");
        UpdateStatus();
    }

    private void NavigateDevice()
    {
        if (_webView.CoreWebView2 == null)
            return;

        _webView.CoreWebView2.Navigate(DeviceUrl);
    }

    private void SendModemInput()
    {
        string text = _modemInput.Text.Trim();
        if (text.Length == 0) return;

        SendFirmwareCommand("MODEM " + text);
        _modemInput.Clear();
    }

    private void SendFirmwareCommand(string command)
    {
        if (_firmwareProcess is not { HasExited: false })
        {
            AppendLog("WARN", "Neběží firmware");
            return;
        }

        try
        {
            _firmwareProcess.StandardInput.WriteLine(command);
            _firmwareProcess.StandardInput.Flush();
            AppendLog("APP", command);
        }
        catch (Exception ex)
        {
            AppendLog("FW", "Send failed: " + ex.Message);
        }
    }

    private void ResetPinRuntimeView()
    {
        foreach (var pin in _pins.Values)
        {
            pin.Mode = -1;
            pin.Value = 1;
        }
        RefreshAllPins();
        ResetRamRuntimeView();
    }

    private void ResetRamRuntimeView()
    {
        _ramTotalBytes = GetBoardRamBytes(_selectedBoard);

        // JSON desky určuje pouze kapacitu RAM.
        // Aktuální obsazení se bere až z běžícího FirmwareSim.exe přes RAMSTATE.
        _ramFreeBytes = _ramTotalBytes;
        _ramMaxBlockBytes = _ramTotalBytes;
        _ramRuntimeLive = false;

        UpdateRamInfoLabel();
        _ramPanel.Invalidate();
    }

    private void UpdateRamState(int total, int free, int maxBlock)
    {
        if (total > 0)
            _ramTotalBytes = total;

        _ramFreeBytes = Math.Clamp(free, 0, Math.Max(1, _ramTotalBytes));
        _ramMaxBlockBytes = Math.Clamp(maxBlock, 0, Math.Max(0, _ramFreeBytes));
        _ramRuntimeLive = true;

        UpdateRamInfoLabel();
        _ramPanel.Invalidate();
    }

    private void UpdateRamInfoLabel()
    {
        int used = Math.Clamp(_ramTotalBytes - _ramFreeBytes, 0, _ramTotalBytes);
        double pct = _ramTotalBytes > 0 ? used * 100.0 / _ramTotalBytes : 0.0;

        if (!_ramRuntimeLive)
        {
            _ramInfoLabel.Text =
                $"RAM kapacita {FormatBytes(_ramTotalBytes)} — čekám na runtime RAMSTATE z firmware";
            return;
        }

        _ramInfoLabel.Text =
            $"RAM obsazeno {FormatBytes(used)} / {FormatBytes(_ramTotalBytes)} ({pct:0.0} %)  " +
            $"free {FormatBytes(_ramFreeBytes)}, max blok {FormatBytes(_ramMaxBlockBytes)}";
    }

    private void RamPanel_Paint(object? sender, PaintEventArgs e)
    {
        e.Graphics.Clear(Color.FromArgb(245, 245, 245));

        int x = 8;
        int y = 24;
        int w = Math.Max(10, _ramPanel.ClientSize.Width - 16);
        int h = 22;

        using var border = new Pen(Color.FromArgb(90, 90, 90));
        using var usedBrush = new SolidBrush(Color.FromArgb(230, 95, 75));
        using var fragBrush = new SolidBrush(Color.FromArgb(245, 180, 70));
        using var freeBrush = new SolidBrush(Color.FromArgb(95, 190, 120));
        using var waitingBrush = new SolidBrush(Color.FromArgb(220, 220, 220));
        using var bgBrush = new SolidBrush(Color.White);

        e.Graphics.FillRectangle(bgBrush, x, y, w, h);

        if (!_ramRuntimeLive)
        {
            e.Graphics.FillRectangle(waitingBrush, x, y, w, h);
            e.Graphics.DrawRectangle(border, x, y, w, h);

            using var waitingFont = new Font("Consolas", 7.5f, FontStyle.Bold);
            using var waitingText = new SolidBrush(Color.FromArgb(80, 80, 80));
            e.Graphics.DrawString("čekám na RAMSTATE z běžícího firmware", waitingFont, waitingText, x + 6, y + 4);
            return;
        }

        int total = Math.Max(1, _ramTotalBytes);
        int used = Math.Clamp(_ramTotalBytes - _ramFreeBytes, 0, _ramTotalBytes);
        int fragFree = Math.Clamp(_ramFreeBytes - _ramMaxBlockBytes, 0, _ramFreeBytes);
        int maxFree = Math.Clamp(_ramMaxBlockBytes, 0, _ramFreeBytes);

        int usedW = (int)Math.Round(w * (used / (double)total));
        int fragW = (int)Math.Round(w * (fragFree / (double)total));
        int maxW = Math.Max(0, w - usedW - fragW);

        if (usedW > 0)
            e.Graphics.FillRectangle(usedBrush, x, y, usedW, h);

        if (fragW > 0)
            e.Graphics.FillRectangle(fragBrush, x + usedW, y, fragW, h);

        if (maxW > 0)
            e.Graphics.FillRectangle(freeBrush, x + usedW + fragW, y, maxW, h);

        e.Graphics.DrawRectangle(border, x, y, w, h);

        using var font = new Font("Consolas", 7.5f);
        using var textBrush = new SolidBrush(Color.FromArgb(20, 20, 20));
        string legend = "červená=obsazeno  oranžová=fragmenty  zelená=max volný blok";
        e.Graphics.DrawString(legend, font, textBrush, x + 2, y + h + 2);
    }

    private static string FormatBytes(int bytes)
    {
        if (bytes >= 1024 * 1024)
            return (bytes / 1024.0 / 1024.0).ToString("0.##", CultureInfo.InvariantCulture) + " MB";

        if (bytes >= 1024)
            return (bytes / 1024.0).ToString("0.#", CultureInfo.InvariantCulture) + " KB";

        return bytes.ToString(CultureInfo.InvariantCulture) + " B";
    }

    private static int GetBoardRamBytes(BoardProfile? board)
    {
        if (board == null)
            return 81920;

        if (board.RamBytes is > 0)
            return board.RamBytes.Value;

        foreach (JsonElement element in new[] { board.RamSize, board.Ram, board.HeapBytes })
        {
            int parsed = ParseMemorySize(element);
            if (parsed > 0)
                return parsed;
        }

        if (board.Mcu.Contains("ESP32", StringComparison.OrdinalIgnoreCase))
            return 327680;

        return 81920;
    }

    private static int ParseMemorySize(JsonElement element)
    {
        return element.ValueKind switch
        {
            JsonValueKind.Number when element.TryGetInt32(out int value) => value,
            JsonValueKind.String => ParseMemorySize(element.GetString()),
            _ => 0
        };
    }

    private static int ParseMemorySize(string? text)
    {
        if (string.IsNullOrWhiteSpace(text))
            return 0;

        string s = text.Trim().Replace(" ", "").Replace(",", ".").ToUpperInvariant();
        var m = Regex.Match(s, @"^(?<num>\d+(?:\.\d+)?)(?<unit>B|KB|K|MB|M)?$");
        if (!m.Success)
            return 0;

        if (!double.TryParse(m.Groups["num"].Value, NumberStyles.Float, CultureInfo.InvariantCulture, out double n))
            return 0;

        string unit = m.Groups["unit"].Value;

        double bytes = unit switch
        {
            "MB" or "M" => n * 1024.0 * 1024.0,
            "KB" or "K" => n * 1024.0,
            _ => n
        };

        if (bytes <= 0 || bytes > int.MaxValue)
            return 0;

        return (int)Math.Round(bytes);
    }

    private void HandleRuntimeLine(string line)
    {
        var m = PinStateRegex.Match(line);
        if (m.Success)
        {
            int gpio = int.Parse(m.Groups["gpio"].Value);
            int mode = int.Parse(m.Groups["mode"].Value);
            int value = int.Parse(m.Groups["value"].Value);
            UpdatePinState(gpio, mode, value);
            return;
        }

        var rm = RamStateRegex.Match(line);
        if (rm.Success)
        {
            int total = int.Parse(rm.Groups["total"].Value, CultureInfo.InvariantCulture);
            int free = int.Parse(rm.Groups["free"].Value, CultureInfo.InvariantCulture);
            int max = int.Parse(rm.Groups["max"].Value, CultureInfo.InvariantCulture);
            UpdateRamState(total, free, max);
            return;
        }

        if (TryHandleDisplayRuntimeLine(line))
            return;

        AppendRaw(line);

        if (line.StartsWith("HTTP listening on ", StringComparison.OrdinalIgnoreCase))
        {
            _ = Task.Delay(250).ContinueWith(_ =>
            {
                if (!IsDisposed)
                    BeginInvoke(() => NavigateDevice());
            });
        }
    }

    private void UpdateStatus()
    {
        bool fw = _firmwareProcess is { HasExited: false };
        _status.Text = $"Firmware: {(fw ? "běží" : "neběží")} | Web: {DeviceUrl}";
        _status.ForeColor = fw ? Color.DarkGreen : Color.DarkRed;
    }

    private void AppendLog(string source, string message)
    {
        AppendRaw($"[{DateTime.Now:HH:mm:ss}] {source}: {message}");
    }

    private void AppendRaw(string line)
    {
        _log.AppendText(line + Environment.NewLine);
    }

    private static string StartPageHtml()
    {
        return """
        <!doctype html>
        <html>
        <head>
          <meta charset="utf-8">
          <style>
            body{font-family:Arial;padding:30px;background:#f5f5f5;color:#111}
            .card{background:white;border-radius:14px;padding:20px;box-shadow:0 2px 12px #0002;max-width:760px;margin:auto}
            code{background:#eee;padding:3px 6px;border-radius:6px}
          </style>
        </head>
        <body>
          <div class="card">
            <h1>ORIScore ESPsim alfa 0.0.1</h1>
            <p>Vyber <code>.ino</code>, dej <b>Build INO</b>, potom <b>Spustit firmware</b>.</p>
            <p>Po startu firmware se sem načte skutečný web z fake <code>WebServer</code>.</p>
          </div>
        </body>
        </html>
        """;
    }

    private sealed class BoardProfile
    {
        [JsonPropertyName("id")]
        public string Id { get; set; } = "";

        [JsonPropertyName("name")]
        public string Name { get; set; } = "";

        [JsonPropertyName("mcu")]
        public string Mcu { get; set; } = "";

        // Velikost RAM/heapu pro náhled paměti.
        // Doporučený formát v board JSON: "ramBytes": 81920
        // Alternativně jde použít i text: "ramSize": "80KB" nebo "ram": "320KB".
        [JsonPropertyName("ramBytes")]
        public int? RamBytes { get; set; }

        [JsonPropertyName("ramSize")]
        public JsonElement RamSize { get; set; }

        [JsonPropertyName("ram")]
        public JsonElement Ram { get; set; }

        [JsonPropertyName("heapBytes")]
        public JsonElement HeapBytes { get; set; }

        [JsonPropertyName("pins")]
        public List<BoardPinProfile> Pins { get; set; } = new();

        [JsonIgnore]
        public string SourceFile { get; set; } = "";

        public override string ToString()
        {
            return string.IsNullOrWhiteSpace(Name) ? Id : Name;
        }
    }

    private sealed class BoardPinProfile
    {
        [JsonPropertyName("boardPin")]
        public string BoardPin { get; set; } = "";

        [JsonPropertyName("label")]
        public string Label
        {
            get => BoardPin;
            set => BoardPin = value;
        }

        [JsonPropertyName("gpio")]
        public int Gpio { get; set; }

        [JsonPropertyName("gpioLabel")]
        public string GpioLabel { get; set; } = "";

        [JsonPropertyName("role")]
        public string Role { get; set; } = "";

        [JsonPropertyName("activeLow")]
        public bool ActiveLow { get; set; }

        [JsonPropertyName("defaultValue")]
        public int? DefaultValue { get; set; }

        [JsonPropertyName("defaultMode")]
        public string? DefaultMode { get; set; }

        [JsonPropertyName("input")]
        public bool Input
        {
            get => string.Equals(DefaultMode, "INPUT_PULLUP", StringComparison.OrdinalIgnoreCase);
            set
            {
                if (value && string.IsNullOrWhiteSpace(DefaultMode))
                    DefaultMode = "INPUT_PULLUP";
            }
        }
    }

    private sealed class PinState
    {
        public PinState(int gpio, string boardPin, string gpioLabel, string role, bool activeLow = false)
        {
            Gpio = gpio;
            BoardPin = boardPin;
            GpioLabel = gpioLabel;
            Role = role;
            ActiveLow = activeLow;
        }

        public int Gpio { get; }
        public string BoardPin { get; }
        public string GpioLabel { get; }
        public string Role { get; }
        public bool ActiveLow { get; }
        public int Mode { get; set; } = -1;
        public int Value { get; set; } = 1;
    }
}

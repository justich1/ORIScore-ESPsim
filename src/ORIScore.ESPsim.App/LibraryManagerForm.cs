namespace ORIScore.ESPsim.App;

public sealed class LibraryManagerForm : Form
{
    private readonly LibraryRepositoryService _service;
    private readonly TabControl _tabs = new() { Dock = DockStyle.Fill };
    private readonly TextBox _detail = new() { Dock = DockStyle.Bottom, Multiline = true, ReadOnly = true, Height = 150, ScrollBars = ScrollBars.Vertical };
    private readonly Button _btnRefresh = new() { Text = "Načíst repo" };
    private readonly Button _btnInstall = new() { Text = "Stáhnout vybrané" };
    private readonly Label _status = new() { Text = "Repo: " + AppPaths.DefaultRepositoryUrl, Dock = DockStyle.Bottom, Height = 24 };

    private readonly Dictionary<string, CheckedListBox> _lists = new(StringComparer.OrdinalIgnoreCase);
    private readonly Dictionary<string, List<RepoPackage>> _items = new(StringComparer.OrdinalIgnoreCase);

    private static readonly (string Key, string Title, string Hint)[] Sections =
    {
        ("libraries", "Knihovny", "Arduino/fake knihovny: .h/.cpp se stáhnou do libraries/installed."),
        ("boards", "Desky", "Board definition JSON soubory se stáhnou do boards."),
        ("devices", "Virtual HW", "JSON definice simulovaných senzorů/periferií se stáhnou do devices.")
    };

    public LibraryManagerForm(LibraryRepositoryService service)
    {
        _service = service;
        Text = "Správce repozitáře ORIScore ESPsim";
        Width = 920;
        Height = 640;
        StartPosition = FormStartPosition.CenterParent;

        var top = new FlowLayoutPanel { Dock = DockStyle.Top, Height = 42, Padding = new Padding(8), FlowDirection = FlowDirection.LeftToRight };
        top.Controls.Add(_btnRefresh);
        top.Controls.Add(_btnInstall);

        foreach (var s in Sections)
        {
            var list = new CheckedListBox { Dock = DockStyle.Fill, CheckOnClick = true };
            list.SelectedIndexChanged += (_, _) => ShowSelectedDetail();
            _lists[s.Key] = list;

            var page = new TabPage(s.Title);
            page.Controls.Add(list);
            page.Controls.Add(new Label
            {
                Text = s.Hint,
                Dock = DockStyle.Top,
                Height = 28,
                Padding = new Padding(8, 7, 8, 0)
            });
            _tabs.TabPages.Add(page);
        }

        Controls.Add(_tabs);
        Controls.Add(_detail);
        Controls.Add(_status);
        Controls.Add(top);

        _btnRefresh.Click += async (_, _) => await LoadRepoAsync();
        _btnInstall.Click += async (_, _) => await InstallSelectedAsync();
        _tabs.SelectedIndexChanged += (_, _) => ShowSelectedDetail();
        Shown += async (_, _) => await LoadRepoAsync();
    }

    private async Task LoadRepoAsync()
    {
        SetBusy(true, "Načítám repo...");
        try
        {
            var data = await _service.GetRepositoryItemsAsync();
            _items.Clear();

            foreach (var s in Sections)
            {
                var list = _lists[s.Key];
                list.Items.Clear();
                var items = data.TryGetValue(s.Key, out var found) ? found : new List<RepoPackage>();
                _items[s.Key] = items;

                foreach (var item in items)
                {
                    string count = item.Files.Count > 0 ? $"{item.Files.Count} souborů" : (item.File ?? "bez souborů");
                    string version = string.IsNullOrWhiteSpace(item.Version) ? "" : $" v{item.Version}";
                    list.Items.Add($"{item.Name}{version}  [{item.Id}]  ({count})", false);
                }
            }

            _status.Text = $"Načteno: knihovny {_items["libraries"].Count}, desky {_items["boards"].Count}, Virtual HW {_items["devices"].Count}";
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, ex.Message, "Správce repozitáře", MessageBoxButtons.OK, MessageBoxIcon.Error);
            _status.Text = "Chyba: " + ex.Message;
        }
        finally { SetBusy(false); }
    }

    private async Task InstallSelectedAsync()
    {
        var selected = new List<RepoPackage>();
        foreach (var s in Sections)
        {
            var list = _lists[s.Key];
            if (!_items.TryGetValue(s.Key, out var items)) continue;
            selected.AddRange(list.CheckedIndices.Cast<int>().Where(i => i >= 0 && i < items.Count).Select(i => items[i]));
        }

        if (selected.Count == 0)
        {
            MessageBox.Show(this, "Nejdřív zaškrtni položky ke stažení.", "Správce repozitáře", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }

        SetBusy(true, "Stahuji vybrané položky...");
        try
        {
            var result = await _service.InstallRepositoryItemsAsync(selected);
            _status.Text = result.Message + " Cíl: " + result.TargetDirectory;
            MessageBox.Show(this, result.Message, "Správce repozitáře", MessageBoxButtons.OK,
                result.Success ? MessageBoxIcon.Information : MessageBoxIcon.Warning);
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, ex.Message, "Správce repozitáře", MessageBoxButtons.OK, MessageBoxIcon.Error);
            _status.Text = "Chyba: " + ex.Message;
        }
        finally { SetBusy(false); }
    }

    private void ShowSelectedDetail()
    {
        if (_tabs.SelectedTab == null) return;
        var section = Sections.FirstOrDefault(x => x.Title == _tabs.SelectedTab.Text).Key;
        if (string.IsNullOrWhiteSpace(section)) return;
        var list = _lists[section];
        int i = list.SelectedIndex;
        if (!_items.TryGetValue(section, out var items) || i < 0 || i >= items.Count) return;

        var item = items[i];
        _detail.Text = $"Typ: {item.Kind}\r\nID: {item.Id}\r\nNázev: {item.Name}\r\nVerze: {item.Version}\r\nKategorie: {item.Category}\r\nManifest: {item.Manifest}\r\nSoubor: {item.File}\r\nPopis: {item.Description}\r\n\r\nSoubory:\r\n" +
                       string.Join("\r\n", item.Files.Select(f => $" - {f.Path} ({f.Type}, {f.Size} B)"));
    }

    private void SetBusy(bool busy, string? text = null)
    {
        _btnRefresh.Enabled = !busy;
        _btnInstall.Enabled = !busy;
        UseWaitCursor = busy;
        if (text != null) _status.Text = text;
    }
}

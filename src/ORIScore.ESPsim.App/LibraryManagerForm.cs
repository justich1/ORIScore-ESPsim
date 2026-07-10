namespace ORIScore.ESPsim.App;

public sealed class LibraryManagerForm : Form
{
    private readonly LibraryRepositoryService _service;
    private readonly TabControl _tabs = new() { Dock = DockStyle.Fill };
    private readonly TextBox _detail = new() { Dock = DockStyle.Bottom, Multiline = true, ReadOnly = true, Height = 165, ScrollBars = ScrollBars.Vertical };
    private readonly Button _btnRefresh = new() { Text = "Načíst repo" };
    private readonly Button _btnInstall = new() { Text = "Instalovat vybrané" };
    private readonly Button _btnUpdate = new() { Text = "Aktualizovat vybrané" };
    private readonly Button _btnUninstall = new() { Text = "Odinstalovat vybrané" };
    private readonly Label _status = new() { Text = "Repo: " + AppPaths.DefaultRepositoryUrl, Dock = DockStyle.Bottom, Height = 24 };

    private readonly Dictionary<string, CheckedListBox> _lists = new(StringComparer.OrdinalIgnoreCase);
    private readonly Dictionary<string, List<RepoPackage>> _items = new(StringComparer.OrdinalIgnoreCase);
    private readonly Dictionary<string, RepositoryItemInstallationInfo> _installation = new(StringComparer.OrdinalIgnoreCase);

    private static readonly (string Key, string Title, string Hint)[] Sections =
    {
        ("libraries", "Knihovny", "Arduino/fake knihovny: instalace, aktualizace a odinstalace v libraries/installed."),
        ("boards", "Desky", "Board definition JSON soubory v adresáři boards."),
        ("devices", "Virtual HW", "JSON definice simulovaných senzorů/periferií v adresáři devices.")
    };

    public LibraryManagerForm(LibraryRepositoryService service)
    {
        _service = service;
        Text = "Správce repozitáře ORIScore ESPsim";
        Width = 1040;
        Height = 680;
        StartPosition = FormStartPosition.CenterParent;

        var top = new FlowLayoutPanel
        {
            Dock = DockStyle.Top,
            Height = 46,
            Padding = new Padding(8),
            FlowDirection = FlowDirection.LeftToRight,
            WrapContents = false
        };
        top.Controls.Add(_btnRefresh);
        top.Controls.Add(_btnInstall);
        top.Controls.Add(_btnUpdate);
        top.Controls.Add(_btnUninstall);

        foreach (var s in Sections)
        {
            var list = new CheckedListBox { Dock = DockStyle.Fill, CheckOnClick = true, HorizontalScrollbar = true };
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
        _btnUpdate.Click += async (_, _) => await UpdateSelectedAsync();
        _btnUninstall.Click += async (_, _) => await UninstallSelectedAsync();
        _tabs.SelectedIndexChanged += (_, _) => ShowSelectedDetail();
        Shown += async (_, _) => await LoadRepoAsync();
    }

    private async Task LoadRepoAsync()
    {
        SetBusy(true, "Načítám repo a kontroluji nainstalované verze...");
        try
        {
            var data = await _service.GetRepositoryItemsAsync();
            _items.Clear();
            _installation.Clear();
            _detail.Clear();

            int updates = 0;
            int installed = 0;

            foreach (var s in Sections)
            {
                var list = _lists[s.Key];
                list.Items.Clear();
                var items = data.TryGetValue(s.Key, out var found) ? found : new List<RepoPackage>();
                _items[s.Key] = items;

                foreach (var item in items)
                {
                    var info = _service.GetInstallationInfo(item);
                    _installation[ItemKey(item)] = info;

                    if (info.IsInstalled) installed++;
                    if (info.State is RepositoryItemState.UpdateAvailable or RepositoryItemState.Damaged) updates++;

                    string count = item.Files.Count > 0 ? $"{item.Files.Count} souborů" : (item.File ?? "bez souborů");
                    string version = string.IsNullOrWhiteSpace(item.Version) ? "" : $" v{item.Version}";
                    list.Items.Add($"{StatePrefix(info.State)} {item.Name}{version}  [{item.Id}]  ({count}) – {info.StatusText}", false);
                }
            }

            _status.Text = $"Repo: knihovny {_items["libraries"].Count}, desky {_items["boards"].Count}, Virtual HW {_items["devices"].Count}. Nainstalováno: {installed}, dostupné aktualizace/opravy: {updates}.";
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, ex.Message, "Správce repozitáře", MessageBoxButtons.OK, MessageBoxIcon.Error);
            _status.Text = "Chyba: " + ex.Message;
        }
        finally
        {
            SetBusy(false);
        }
    }

    private async Task InstallSelectedAsync()
    {
        var checkedItems = GetCheckedItems();
        var selected = checkedItems
            .Where(x => !x.Info.IsInstalled)
            .Select(x => x.Item)
            .ToList();

        if (checkedItems.Count == 0)
        {
            MessageBox.Show(this, "Nejdřív zaškrtni položky k instalaci.", "Správce repozitáře", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }

        if (selected.Count == 0)
        {
            MessageBox.Show(this, "Všechny zaškrtnuté položky už jsou nainstalované. Použij Aktualizovat vybrané.", "Správce repozitáře", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }

        bool reload = false;
        SetBusy(true, "Instaluji vybrané položky...");
        try
        {
            var result = await _service.InstallRepositoryItemsAsync(selected);
            reload = true;
            string skipped = checkedItems.Count > selected.Count ? $" Přeskočeno již nainstalovaných: {checkedItems.Count - selected.Count}." : "";
            _status.Text = result.Message + skipped;
            MessageBox.Show(this, result.Message + skipped, "Správce repozitáře", MessageBoxButtons.OK,
                result.Success ? MessageBoxIcon.Information : MessageBoxIcon.Warning);
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, ex.Message, "Správce repozitáře", MessageBoxButtons.OK, MessageBoxIcon.Error);
            _status.Text = "Chyba: " + ex.Message;
        }
        finally
        {
            SetBusy(false);
        }

        if (reload)
            await LoadRepoAsync();
    }

    private async Task UpdateSelectedAsync()
    {
        var checkedItems = GetCheckedItems();
        var selected = checkedItems
            .Where(x => x.Info.IsInstalled)
            .Select(x => x.Item)
            .ToList();

        if (checkedItems.Count == 0)
        {
            MessageBox.Show(this, "Nejdřív zaškrtni položky k aktualizaci.", "Správce repozitáře", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }

        if (selected.Count == 0)
        {
            MessageBox.Show(this, "Zaškrtnuté položky ještě nejsou nainstalované. Použij Instalovat vybrané.", "Správce repozitáře", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }

        int newerLocal = checkedItems.Count(x => x.Info.State == RepositoryItemState.InstalledNewer);
        if (newerLocal > 0)
        {
            var answer = MessageBox.Show(this,
                $"U {newerLocal} položek je lokálně novější verze než v repozitáři. Aktualizace je nahradí verzí z repozitáře. Pokračovat?",
                "Potvrdit nahrazení novější verze",
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Warning);
            if (answer != DialogResult.Yes) return;
        }

        bool reload = false;
        SetBusy(true, "Aktualizuji vybrané položky...");
        try
        {
            var result = await _service.UpdateRepositoryItemsAsync(selected);
            reload = true;
            string skipped = checkedItems.Count > selected.Count ? $" Přeskočeno nenainstalovaných: {checkedItems.Count - selected.Count}." : "";
            _status.Text = result.Message + skipped;
            MessageBox.Show(this, result.Message + skipped, "Správce repozitáře", MessageBoxButtons.OK,
                result.Success ? MessageBoxIcon.Information : MessageBoxIcon.Warning);
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, ex.Message, "Správce repozitáře", MessageBoxButtons.OK, MessageBoxIcon.Error);
            _status.Text = "Chyba: " + ex.Message;
        }
        finally
        {
            SetBusy(false);
        }

        if (reload)
            await LoadRepoAsync();
    }

    private async Task UninstallSelectedAsync()
    {
        var checkedItems = GetCheckedItems();
        var selected = checkedItems
            .Where(x => x.Info.IsInstalled)
            .Select(x => x.Item)
            .ToList();

        if (checkedItems.Count == 0)
        {
            MessageBox.Show(this, "Nejdřív zaškrtni položky k odinstalaci.", "Správce repozitáře", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }

        if (selected.Count == 0)
        {
            MessageBox.Show(this, "Žádná ze zaškrtnutých položek není nainstalovaná.", "Správce repozitáře", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }

        string names = string.Join("\r\n", selected.Take(12).Select(x => "• " + x.Name + " [" + x.Id + "]"));
        if (selected.Count > 12)
            names += $"\r\n• … a dalších {selected.Count - 12}";

        var answer = MessageBox.Show(this,
            $"Opravdu odinstalovat {selected.Count} položek?\r\n\r\n{names}",
            "Potvrdit odinstalaci",
            MessageBoxButtons.YesNo,
            MessageBoxIcon.Warning);
        if (answer != DialogResult.Yes) return;

        bool reload = false;
        SetBusy(true, "Odinstalovávám vybrané položky...");
        try
        {
            var result = await _service.UninstallRepositoryItemsAsync(selected);
            reload = true;
            string skipped = checkedItems.Count > selected.Count ? $" Přeskočeno nenainstalovaných: {checkedItems.Count - selected.Count}." : "";
            _status.Text = result.Message + skipped;
            MessageBox.Show(this, result.Message + skipped, "Správce repozitáře", MessageBoxButtons.OK,
                result.Success ? MessageBoxIcon.Information : MessageBoxIcon.Warning);
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, ex.Message, "Správce repozitáře", MessageBoxButtons.OK, MessageBoxIcon.Error);
            _status.Text = "Chyba: " + ex.Message;
        }
        finally
        {
            SetBusy(false);
        }

        if (reload)
            await LoadRepoAsync();
    }

    private List<(RepoPackage Item, RepositoryItemInstallationInfo Info)> GetCheckedItems()
    {
        var selected = new List<(RepoPackage, RepositoryItemInstallationInfo)>();

        foreach (var s in Sections)
        {
            var list = _lists[s.Key];
            if (!_items.TryGetValue(s.Key, out var items)) continue;

            foreach (int index in list.CheckedIndices.Cast<int>())
            {
                if (index < 0 || index >= items.Count) continue;
                var item = items[index];
                if (!_installation.TryGetValue(ItemKey(item), out var info))
                    info = _service.GetInstallationInfo(item);
                selected.Add((item, info));
            }
        }

        return selected;
    }

    private void ShowSelectedDetail()
    {
        if (_tabs.SelectedTab == null) return;
        var section = Sections.FirstOrDefault(x => x.Title == _tabs.SelectedTab.Text).Key;
        if (string.IsNullOrWhiteSpace(section)) return;
        var list = _lists[section];
        int i = list.SelectedIndex;
        if (!_items.TryGetValue(section, out var items) || i < 0 || i >= items.Count)
        {
            _detail.Clear();
            return;
        }

        var item = items[i];
        if (!_installation.TryGetValue(ItemKey(item), out var info))
            info = _service.GetInstallationInfo(item);

        _detail.Text =
            $"Stav: {info.StatusText}\r\n" +
            $"Nainstalovaná verze: {info.InstalledVersion ?? "-"}\r\n" +
            $"Verze v repozitáři: {item.Version}\r\n" +
            $"Typ: {item.Kind}\r\n" +
            $"ID: {item.Id}\r\n" +
            $"Název: {item.Name}\r\n" +
            $"Kategorie: {item.Category}\r\n" +
            $"Manifest: {item.Manifest}\r\n" +
            $"Soubor: {item.File}\r\n" +
            $"Popis: {item.Description}\r\n\r\n" +
            "Soubory:\r\n" +
            string.Join("\r\n", item.Files.Select(f => $" - {f.Path} ({f.Type}, {f.Size} B)"));
    }

    private void SetBusy(bool busy, string? text = null)
    {
        _btnRefresh.Enabled = !busy;
        _btnInstall.Enabled = !busy;
        _btnUpdate.Enabled = !busy;
        _btnUninstall.Enabled = !busy;
        UseWaitCursor = busy;
        if (text != null) _status.Text = text;
    }

    private static string ItemKey(RepoPackage item) => item.Kind + "\n" + item.Id;

    private static string StatePrefix(RepositoryItemState state) => state switch
    {
        RepositoryItemState.NotInstalled => "[NENÍ]",
        RepositoryItemState.UpToDate => "[AKTUÁLNÍ]",
        RepositoryItemState.UpdateAvailable => "[AKTUALIZACE]",
        RepositoryItemState.InstalledNewer => "[NOVĚJŠÍ LOKÁLNÍ]",
        RepositoryItemState.Damaged => "[OPRAVIT]",
        _ => "[NAINSTALOVÁNO]"
    };
}

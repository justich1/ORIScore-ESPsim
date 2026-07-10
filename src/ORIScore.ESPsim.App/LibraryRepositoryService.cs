using System.Security.Cryptography;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace ORIScore.ESPsim.App;

public sealed record RepositorySyncResult(bool Success, string Message, string TargetDirectory);
public sealed record RepoFileEntry(string Path, string Type, long Size, string? Sha256);
public sealed record RepoPackage(string Kind, string Id, string Name, string Version, string? Description, string? Category, string? Manifest, string? File, List<RepoFileEntry> Files);
public sealed record LibraryFileEntry(string Path, string Type, long Size, string? Sha256);
public sealed record LibraryPackage(string Id, string Name, string Version, string? Description, string? Category, List<LibraryFileEntry> Files);
public sealed record LibraryInstallResult(bool Success, string Message, string TargetDirectory);

public enum RepositoryItemState
{
    NotInstalled,
    UpToDate,
    UpdateAvailable,
    Installed,
    InstalledNewer,
    Damaged
}

public sealed record RepositoryItemInstallationInfo(
    RepositoryItemState State,
    string? InstalledVersion,
    string StatusText)
{
    public bool IsInstalled => State != RepositoryItemState.NotInstalled;
}

public sealed class LibraryRepositoryService
{
    private static readonly string[] DefaultIndexes =
    {
        "manifest.json",
        "libraries/index.json",
        "boards/index.json",
        "devices/index.json"
    };

    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        PropertyNameCaseInsensitive = true,
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
        WriteIndented = true
    };

    public async Task<RepositorySyncResult> SyncAsync(string? repositoryUrl = null, CancellationToken cancellationToken = default)
    {
        AppPaths.EnsurePortableLayout();

        string baseUrl = NormalizeBaseUrl(repositoryUrl ?? LoadConfiguredRepositoryUrl());
        string target = AppPaths.LibraryRepoOnlineDir;
        Directory.CreateDirectory(target);

        using var http = new HttpClient { Timeout = TimeSpan.FromSeconds(30) };
        int ok = 0;
        var errors = new List<string>();

        foreach (string index in DefaultIndexes)
        {
            string url = baseUrl + index;
            string outFile = Path.Combine(target, index.Replace('/', Path.DirectorySeparatorChar));
            Directory.CreateDirectory(Path.GetDirectoryName(outFile)!);

            try
            {
                using var response = await http.GetAsync(url, cancellationToken);
                if (!response.IsSuccessStatusCode)
                {
                    errors.Add($"{index}: HTTP {(int)response.StatusCode}");
                    continue;
                }

                string text = await response.Content.ReadAsStringAsync(cancellationToken);
                JsonDocument.Parse(text).Dispose();
                await File.WriteAllTextAsync(outFile, text, cancellationToken);
                ok++;
            }
            catch (Exception ex)
            {
                errors.Add($"{index}: {ex.Message}");
            }
        }

        string msg = errors.Count == 0
            ? $"Repo synchronizováno: {ok} indexů z {baseUrl}"
            : $"Repo částečně synchronizováno: {ok} OK, chyby: {string.Join(" | ", errors)}";

        return new RepositorySyncResult(ok > 0, msg, target);
    }

    public async Task<Dictionary<string, List<RepoPackage>>> GetRepositoryItemsAsync(string? repositoryUrl = null, CancellationToken cancellationToken = default)
    {
        string baseUrl = NormalizeBaseUrl(repositoryUrl ?? LoadConfiguredRepositoryUrl());
        using var http = new HttpClient { Timeout = TimeSpan.FromSeconds(30) };

        var result = new Dictionary<string, List<RepoPackage>>(StringComparer.OrdinalIgnoreCase)
        {
            ["libraries"] = await GetPackageItemsAsync(http, baseUrl, "libraries", "library.json", cancellationToken),
            ["boards"] = await GetJsonFileItemsAsync(http, baseUrl, "boards", cancellationToken),
            ["devices"] = await GetJsonFileItemsAsync(http, baseUrl, "devices", cancellationToken)
        };

        return result;
    }

    public async Task<List<LibraryPackage>> GetLibrariesAsync(string? repositoryUrl = null, CancellationToken cancellationToken = default)
    {
        var all = await GetRepositoryItemsAsync(repositoryUrl, cancellationToken);
        return all["libraries"].Select(x => new LibraryPackage(
            x.Id, x.Name, x.Version, x.Description, x.Category,
            x.Files.Select(f => new LibraryFileEntry(f.Path, f.Type, f.Size, f.Sha256)).ToList()
        )).ToList();
    }

    public RepositoryItemInstallationInfo GetInstallationInfo(RepoPackage item)
    {
        try
        {
            return item.Kind switch
            {
                "libraries" => GetLibraryInstallationInfo(item),
                "boards" or "devices" => GetJsonItemInstallationInfo(item),
                _ => new RepositoryItemInstallationInfo(RepositoryItemState.NotInstalled, null, "nenainstalováno")
            };
        }
        catch (Exception ex)
        {
            return new RepositoryItemInstallationInfo(RepositoryItemState.Damaged, null, "stav nelze ověřit: " + ex.Message);
        }
    }

    public Task<LibraryInstallResult> InstallRepositoryItemsAsync(
        IEnumerable<RepoPackage> items,
        string? repositoryUrl = null,
        CancellationToken cancellationToken = default)
        => InstallOrUpdateRepositoryItemsAsync(items, repositoryUrl, cancellationToken);

    public Task<LibraryInstallResult> UpdateRepositoryItemsAsync(
        IEnumerable<RepoPackage> items,
        string? repositoryUrl = null,
        CancellationToken cancellationToken = default)
        => InstallOrUpdateRepositoryItemsAsync(items, repositoryUrl, cancellationToken);

    public Task<LibraryInstallResult> UninstallRepositoryItemsAsync(
        IEnumerable<RepoPackage> items,
        CancellationToken cancellationToken = default)
    {
        AppPaths.EnsurePortableLayout();

        int removedItems = 0;
        int removedFiles = 0;
        var errors = new List<string>();

        foreach (var item in items)
        {
            cancellationToken.ThrowIfCancellationRequested();

            try
            {
                if (item.Kind == "libraries")
                {
                    string targetDir = GetInstalledLibraryDirectory(item.Id);
                    if (!Directory.Exists(targetDir))
                        continue;

                    removedFiles += Directory.GetFiles(targetDir, "*", SearchOption.AllDirectories).Length;
                    Directory.Delete(targetDir, recursive: true);
                    removedItems++;
                    continue;
                }

                if (item.Kind is "boards" or "devices")
                {
                    string? targetFile = GetInstalledJsonItemPath(item);
                    if (string.IsNullOrWhiteSpace(targetFile) || !File.Exists(targetFile))
                        continue;

                    File.Delete(targetFile);
                    removedFiles++;
                    removedItems++;
                }
            }
            catch (Exception ex)
            {
                errors.Add($"{item.Kind}/{item.Id}: {ex.Message}");
            }
        }

        string message = errors.Count == 0
            ? $"Odinstalováno {removedItems} položek ({removedFiles} souborů)."
            : $"Odinstalováno {removedItems} položek ({removedFiles} souborů), chyby: {string.Join(" | ", errors)}";

        return Task.FromResult(new LibraryInstallResult(errors.Count == 0, message, AppPaths.BaseDir));
    }

    public async Task<LibraryInstallResult> InstallLibrariesAsync(IEnumerable<LibraryPackage> libraries, string? repositoryUrl = null, CancellationToken cancellationToken = default)
    {
        var items = libraries.Select(x => new RepoPackage("libraries", x.Id, x.Name, x.Version, x.Description, x.Category, $"libraries/{x.Id}/library.json", null,
            x.Files.Select(f => new RepoFileEntry(f.Path, f.Type, f.Size, f.Sha256)).ToList()));
        return await InstallRepositoryItemsAsync(items, repositoryUrl, cancellationToken);
    }

    private async Task<LibraryInstallResult> InstallOrUpdateRepositoryItemsAsync(
        IEnumerable<RepoPackage> items,
        string? repositoryUrl,
        CancellationToken cancellationToken)
    {
        AppPaths.EnsurePortableLayout();
        string baseUrl = NormalizeBaseUrl(repositoryUrl ?? LoadConfiguredRepositoryUrl());
        using var http = new HttpClient { Timeout = TimeSpan.FromSeconds(60) };

        int installedItems = 0;
        int files = 0;
        var errors = new List<string>();

        foreach (var item in items)
        {
            cancellationToken.ThrowIfCancellationRequested();

            try
            {
                if (item.Kind is "boards" or "devices")
                {
                    int downloaded = await InstallJsonItemAsync(http, baseUrl, item, cancellationToken);
                    files += downloaded;
                    installedItems++;
                    continue;
                }

                if (item.Kind == "libraries")
                {
                    int downloaded = await InstallLibraryPackageAsync(http, baseUrl, item, cancellationToken);
                    files += downloaded;
                    installedItems++;
                }
            }
            catch (Exception ex)
            {
                errors.Add($"{item.Kind}/{item.Id}: {ex.Message}");
            }
        }

        string msg = errors.Count == 0
            ? $"Nainstalováno nebo aktualizováno {installedItems} položek ({files} souborů)."
            : $"Nainstalováno nebo aktualizováno {installedItems} položek ({files} souborů), chyby: {string.Join(" | ", errors)}";

        return new LibraryInstallResult(errors.Count == 0, msg, AppPaths.BaseDir);
    }

    private static async Task<int> InstallJsonItemAsync(
        HttpClient http,
        string baseUrl,
        RepoPackage item,
        CancellationToken cancellationToken)
    {
        if (string.IsNullOrWhiteSpace(item.File))
            throw new InvalidDataException("V manifestu chybí název souboru.");

        string rel = NormalizeRelativeFile(item.File);
        if (string.IsNullOrWhiteSpace(rel))
            throw new InvalidDataException("Neplatná cesta souboru.");

        string sourceUrl = baseUrl + item.Kind + "/" + rel.Replace('\\', '/');
        string targetDir = item.Kind == "boards" ? AppPaths.BoardsDir : AppPaths.DevicesDir;
        Directory.CreateDirectory(targetDir);

        string outFile = Path.Combine(targetDir, Path.GetFileName(rel));
        byte[] data = await http.GetByteArrayAsync(sourceUrl, cancellationToken);

        RepoFileEntry? repoFile = item.Files.FirstOrDefault();
        VerifyHash(data, repoFile?.Sha256, $"{item.Kind}/{item.Id}/{rel}");

        string tempFile = outFile + ".download-" + Guid.NewGuid().ToString("N");
        try
        {
            await File.WriteAllBytesAsync(tempFile, data, cancellationToken);
            File.Move(tempFile, outFile, overwrite: true);
        }
        finally
        {
            TryDeleteFile(tempFile);
        }

        return 1;
    }

    private static async Task<int> InstallLibraryPackageAsync(
        HttpClient http,
        string baseUrl,
        RepoPackage item,
        CancellationToken cancellationToken)
    {
        if (item.Files.Count == 0)
            throw new InvalidDataException("Manifest knihovny neobsahuje žádné soubory.");

        string safeId = SafePathPart(item.Id);
        string stagingRoot = Path.Combine(AppPaths.LibraryRepoDir, "staging");
        Directory.CreateDirectory(stagingRoot);

        string stageDir = Path.Combine(stagingRoot, safeId + "-" + Guid.NewGuid().ToString("N"));
        string targetDir = GetInstalledLibraryDirectory(item.Id);
        string remoteDir = GetRemotePackageDirectory(item);
        Directory.CreateDirectory(stageDir);

        int downloaded = 0;
        try
        {
            foreach (var f in item.Files)
            {
                cancellationToken.ThrowIfCancellationRequested();

                string rel = NormalizeRelativeFile(f.Path);
                if (string.IsNullOrWhiteSpace(rel))
                    throw new InvalidDataException($"Neplatná cesta v manifestu: {f.Path}");

                string sourceUrl = baseUrl + remoteDir + "/" + rel.Replace('\\', '/');
                string outFile = Path.Combine(stageDir, rel.Replace('/', Path.DirectorySeparatorChar));
                Directory.CreateDirectory(Path.GetDirectoryName(outFile)!);

                byte[] data = await http.GetByteArrayAsync(sourceUrl, cancellationToken);
                VerifyHash(data, f.Sha256, $"{item.Kind}/{item.Id}/{rel}");
                await File.WriteAllBytesAsync(outFile, data, cancellationToken);
                downloaded++;
            }

            var installedManifest = new
            {
                id = item.Id,
                name = item.Name,
                version = item.Version,
                description = item.Description,
                category = item.Category,
                installedAt = DateTimeOffset.Now,
                files = item.Files.Select(f => new
                {
                    path = f.Path,
                    type = f.Type,
                    size = f.Size,
                    sha256 = f.Sha256
                }).ToList()
            };

            await File.WriteAllTextAsync(
                Path.Combine(stageDir, "library.json"),
                JsonSerializer.Serialize(installedManifest, JsonOptions),
                cancellationToken);

            ReplaceDirectory(stageDir, targetDir);
            return downloaded;
        }
        finally
        {
            TryDeleteDirectory(stageDir);
        }
    }

    private RepositoryItemInstallationInfo GetLibraryInstallationInfo(RepoPackage item)
    {
        string rootDir = GetInstalledLibraryDirectory(item.Id);
        if (!Directory.Exists(rootDir))
            return new RepositoryItemInstallationInfo(RepositoryItemState.NotInstalled, null, "nenainstalováno");

        string manifestPath = Path.Combine(rootDir, "library.json");
        string? installedVersion = ReadJsonStringIgnoreCase(manifestPath, "version");

        bool missingFile = false;
        bool hashMismatch = false;
        bool hasHash = false;

        foreach (var f in item.Files)
        {
            string rel = NormalizeRelativeFile(f.Path);
            if (string.IsNullOrWhiteSpace(rel))
            {
                missingFile = true;
                continue;
            }

            string localFile = Path.Combine(rootDir, rel.Replace('/', Path.DirectorySeparatorChar));
            if (!File.Exists(localFile))
            {
                missingFile = true;
                continue;
            }

            if (!string.IsNullOrWhiteSpace(f.Sha256))
            {
                hasHash = true;
                string localHash = ComputeFileSha256(localFile);
                if (!localHash.Equals(f.Sha256, StringComparison.OrdinalIgnoreCase))
                    hashMismatch = true;
            }
        }

        if (missingFile)
            return new RepositoryItemInstallationInfo(RepositoryItemState.Damaged, installedVersion, "neúplná instalace – lze opravit aktualizací");

        if (hashMismatch)
            return new RepositoryItemInstallationInfo(RepositoryItemState.UpdateAvailable, installedVersion, "lokální soubory se liší – lze aktualizovat");

        if (!string.IsNullOrWhiteSpace(installedVersion) && !string.IsNullOrWhiteSpace(item.Version))
        {
            int compare = CompareVersions(installedVersion, item.Version);
            if (compare < 0)
                return new RepositoryItemInstallationInfo(RepositoryItemState.UpdateAvailable, installedVersion, $"aktualizace {installedVersion} → {item.Version}");
            if (compare > 0)
                return new RepositoryItemInstallationInfo(RepositoryItemState.InstalledNewer, installedVersion, $"lokálně novější verze {installedVersion}");

            return new RepositoryItemInstallationInfo(RepositoryItemState.UpToDate, installedVersion, "aktuální");
        }

        if (hasHash && item.Files.Count > 0)
            return new RepositoryItemInstallationInfo(RepositoryItemState.UpToDate, installedVersion, "aktuální podle kontrolních součtů");

        return new RepositoryItemInstallationInfo(RepositoryItemState.Installed, installedVersion, "nainstalováno – verzi nelze porovnat");
    }

    private RepositoryItemInstallationInfo GetJsonItemInstallationInfo(RepoPackage item)
    {
        string? localFile = GetInstalledJsonItemPath(item);
        if (string.IsNullOrWhiteSpace(localFile) || !File.Exists(localFile))
            return new RepositoryItemInstallationInfo(RepositoryItemState.NotInstalled, null, "nenainstalováno");

        string? installedVersion = ReadJsonStringIgnoreCase(localFile, "version");
        RepoFileEntry? repoFile = item.Files.FirstOrDefault();

        if (!string.IsNullOrWhiteSpace(repoFile?.Sha256))
        {
            string localHash = ComputeFileSha256(localFile);
            if (localHash.Equals(repoFile.Sha256, StringComparison.OrdinalIgnoreCase))
                return new RepositoryItemInstallationInfo(RepositoryItemState.UpToDate, installedVersion, "aktuální");

            return new RepositoryItemInstallationInfo(RepositoryItemState.UpdateAvailable, installedVersion, "soubor se liší od repozitáře");
        }

        if (!string.IsNullOrWhiteSpace(installedVersion) && !string.IsNullOrWhiteSpace(item.Version))
        {
            int compare = CompareVersions(installedVersion, item.Version);
            if (compare < 0)
                return new RepositoryItemInstallationInfo(RepositoryItemState.UpdateAvailable, installedVersion, $"aktualizace {installedVersion} → {item.Version}");
            if (compare > 0)
                return new RepositoryItemInstallationInfo(RepositoryItemState.InstalledNewer, installedVersion, $"lokálně novější verze {installedVersion}");

            return new RepositoryItemInstallationInfo(RepositoryItemState.UpToDate, installedVersion, "aktuální");
        }

        return new RepositoryItemInstallationInfo(RepositoryItemState.Installed, installedVersion, "nainstalováno – verzi nelze porovnat");
    }

    private static async Task<List<RepoPackage>> GetPackageItemsAsync(HttpClient http, string baseUrl, string kind, string defaultManifestName, CancellationToken ct)
    {
        var list = new List<RepoPackage>();
        string indexJson = await http.GetStringAsync(baseUrl + kind + "/index.json", ct);
        using var doc = JsonDocument.Parse(indexJson);
        if (!doc.RootElement.TryGetProperty(kind, out var arr) || arr.ValueKind != JsonValueKind.Array)
            return list;

        foreach (var item in arr.EnumerateArray())
        {
            string id = GetString(item, "id");
            if (string.IsNullOrWhiteSpace(id)) continue;
            string name = GetString(item, "name", id);
            string ver = GetString(item, "version");
            string? desc = GetNullableString(item, "description");
            string? cat = GetNullableString(item, "category");
            string manifest = GetString(item, "manifest");

            if (!string.IsNullOrWhiteSpace(manifest))
            {
                try
                {
                    string json = await http.GetStringAsync(baseUrl + manifest.TrimStart('/'), ct);
                    var parsed = ParsePackage(kind, json, id, manifest);
                    if (parsed != null) list.Add(parsed);
                    continue;
                }
                catch
                {
                    list.Add(new RepoPackage(kind, id, name, ver, "Manifest nejde načíst", cat, manifest, null, new List<RepoFileEntry>()));
                    continue;
                }
            }

            string file = GetString(item, "file");
            if (!string.IsNullOrWhiteSpace(file))
            {
                list.Add(new RepoPackage(kind, id, name, ver, desc, cat, null, file, new List<RepoFileEntry>()));
                continue;
            }

            string fallbackManifest = $"{kind}/{id}/{defaultManifestName}";
            try
            {
                string json = await http.GetStringAsync(baseUrl + fallbackManifest, ct);
                var parsed = ParsePackage(kind, json, id, fallbackManifest);
                if (parsed != null) list.Add(parsed);
            }
            catch
            {
                list.Add(new RepoPackage(kind, id, name, ver, desc ?? "Manifest nenalezen", cat, fallbackManifest, null, new List<RepoFileEntry>()));
            }
        }

        return list.OrderBy(x => x.Category).ThenBy(x => x.Name).ToList();
    }

    private static async Task<List<RepoPackage>> GetJsonFileItemsAsync(HttpClient http, string baseUrl, string kind, CancellationToken ct)
    {
        var list = new List<RepoPackage>();
        string indexJson = await http.GetStringAsync(baseUrl + kind + "/index.json", ct);
        using var doc = JsonDocument.Parse(indexJson);
        if (!doc.RootElement.TryGetProperty(kind, out var arr) || arr.ValueKind != JsonValueKind.Array)
            return list;

        foreach (var item in arr.EnumerateArray())
        {
            string file = GetString(item, "file");
            string id = GetString(item, "id", Path.GetFileNameWithoutExtension(file));
            string name = GetString(item, "name", id);
            string ver = GetString(item, "version");
            string? desc = GetNullableString(item, "description");
            long size = item.TryGetProperty("size", out var sEl) && sEl.TryGetInt64(out long sz) ? sz : 0;
            var files = string.IsNullOrWhiteSpace(file) ? new List<RepoFileEntry>() : new List<RepoFileEntry> { new(file, "json", size, GetNullableString(item, "sha256")) };
            list.Add(new RepoPackage(kind, id, name, ver, desc, kind, null, file, files));
        }
        return list.OrderBy(x => x.Name).ToList();
    }

    private static RepoPackage? ParsePackage(string kind, string json, string fallbackId, string manifest)
    {
        using var doc = JsonDocument.Parse(json);
        var r = doc.RootElement;
        string id = GetString(r, "id", fallbackId);
        string name = GetString(r, "name", id);
        string version = GetString(r, "version");
        string? desc = GetNullableString(r, "description");
        string? cat = GetNullableString(r, "category");

        var files = new List<RepoFileEntry>();
        if (r.TryGetProperty("files", out var fs) && fs.ValueKind == JsonValueKind.Array)
        {
            foreach (var f in fs.EnumerateArray())
            {
                string path = GetString(f, "path");
                if (string.IsNullOrWhiteSpace(path)) continue;
                string type = GetString(f, "type", "file");
                long size = f.TryGetProperty("size", out var sEl) && sEl.TryGetInt64(out long sz) ? sz : 0;
                string? sha = GetNullableString(f, "sha256");
                files.Add(new RepoFileEntry(path, type, size, sha));
            }
        }

        return new RepoPackage(kind, id, name, version, desc, cat, manifest, null, files);
    }

    private static string GetString(JsonElement e, string name, string fallback = "")
        => e.TryGetProperty(name, out var p) && p.ValueKind == JsonValueKind.String ? p.GetString() ?? fallback : fallback;

    private static string? GetNullableString(JsonElement e, string name)
        => e.TryGetProperty(name, out var p) && p.ValueKind == JsonValueKind.String ? p.GetString() : null;

    private static string LoadConfiguredRepositoryUrl()
    {
        string config = Path.Combine(AppPaths.ConfigsDir, "repository.json");
        if (!File.Exists(config)) return AppPaths.DefaultRepositoryUrl;
        try
        {
            using var doc = JsonDocument.Parse(File.ReadAllText(config));
            if (doc.RootElement.TryGetProperty("defaultRepositoryUrl", out var urlEl))
            {
                string? url = urlEl.GetString();
                if (!string.IsNullOrWhiteSpace(url)) return url;
            }
        }
        catch { }
        return AppPaths.DefaultRepositoryUrl;
    }

    private static string NormalizeBaseUrl(string url)
    {
        url = string.IsNullOrWhiteSpace(url) ? AppPaths.DefaultRepositoryUrl : url.Trim();
        return url.EndsWith('/') ? url : url + "/";
    }

    private static string NormalizeRelativeFile(string path)
    {
        path = path.Replace('\\', '/').Trim('/');
        if (path.Contains("..", StringComparison.Ordinal)) return "";
        return path;
    }

    private static string SafePathPart(string value)
    {
        var chars = value.Where(c => char.IsLetterOrDigit(c) || c is '_' or '-' or '.').ToArray();
        return chars.Length == 0 ? "item" : new string(chars);
    }

    private static string GetInstalledLibraryDirectory(string id)
        => Path.Combine(AppPaths.LibraryInstalledDir, SafePathPart(id));

    private static string? GetInstalledJsonItemPath(RepoPackage item)
    {
        if (string.IsNullOrWhiteSpace(item.File)) return null;
        string fileName = Path.GetFileName(item.File);
        if (string.IsNullOrWhiteSpace(fileName)) return null;

        string targetDir = item.Kind == "boards" ? AppPaths.BoardsDir : AppPaths.DevicesDir;
        return Path.Combine(targetDir, fileName);
    }

    private static string GetRemotePackageDirectory(RepoPackage item)
    {
        string manifest = NormalizeRelativeFile(item.Manifest ?? "");
        int slash = manifest.LastIndexOf('/');
        if (slash > 0)
            return manifest[..slash];

        return item.Kind + "/" + SafePathPart(item.Id);
    }

    private static void VerifyHash(byte[] data, string? expectedSha256, string label)
    {
        if (string.IsNullOrWhiteSpace(expectedSha256)) return;

        string hash = Convert.ToHexString(SHA256.HashData(data)).ToLowerInvariant();
        if (!hash.Equals(expectedSha256, StringComparison.OrdinalIgnoreCase))
            throw new InvalidDataException($"{label}: SHA256 nesedí");
    }

    private static string ComputeFileSha256(string file)
    {
        using var stream = File.OpenRead(file);
        return Convert.ToHexString(SHA256.HashData(stream)).ToLowerInvariant();
    }

    private static string? ReadJsonStringIgnoreCase(string file, string propertyName)
    {
        if (!File.Exists(file)) return null;

        try
        {
            using var doc = JsonDocument.Parse(File.ReadAllText(file));
            foreach (var property in doc.RootElement.EnumerateObject())
            {
                if (property.Name.Equals(propertyName, StringComparison.OrdinalIgnoreCase) &&
                    property.Value.ValueKind == JsonValueKind.String)
                {
                    return property.Value.GetString();
                }
            }
        }
        catch { }

        return null;
    }

    private static int CompareVersions(string installed, string repository)
    {
        if (installed.Equals(repository, StringComparison.OrdinalIgnoreCase))
            return 0;

        long[] left = Regex.Matches(installed, @"\d+")
            .Cast<Match>()
            .Select(m => long.TryParse(m.Value, out long value) ? value : 0)
            .ToArray();
        long[] right = Regex.Matches(repository, @"\d+")
            .Cast<Match>()
            .Select(m => long.TryParse(m.Value, out long value) ? value : 0)
            .ToArray();

        int max = Math.Max(left.Length, right.Length);
        for (int i = 0; i < max; i++)
        {
            long a = i < left.Length ? left[i] : 0;
            long b = i < right.Length ? right[i] : 0;
            int compare = a.CompareTo(b);
            if (compare != 0) return compare;
        }

        return 0;
    }

    private static void ReplaceDirectory(string stagedDirectory, string targetDirectory)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(targetDirectory)!);
        string backupRoot = Path.Combine(AppPaths.LibraryRepoDir, "staging");
        Directory.CreateDirectory(backupRoot);
        string backupDirectory = Path.Combine(
            backupRoot,
            Path.GetFileName(targetDirectory) + "-backup-" + Guid.NewGuid().ToString("N"));
        bool backupCreated = false;

        if (Directory.Exists(targetDirectory))
        {
            Directory.Move(targetDirectory, backupDirectory);
            backupCreated = true;
        }

        try
        {
            Directory.Move(stagedDirectory, targetDirectory);
        }
        catch
        {
            if (!Directory.Exists(targetDirectory) && backupCreated && Directory.Exists(backupDirectory))
                Directory.Move(backupDirectory, targetDirectory);
            throw;
        }

        if (backupCreated)
            TryDeleteDirectory(backupDirectory);
    }

    private static void TryDeleteDirectory(string directory)
    {
        try
        {
            if (Directory.Exists(directory))
                Directory.Delete(directory, recursive: true);
        }
        catch { }
    }

    private static void TryDeleteFile(string file)
    {
        try
        {
            if (File.Exists(file))
                File.Delete(file);
        }
        catch { }
    }
}

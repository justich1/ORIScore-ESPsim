using System.Security.Cryptography;
using System.Text.Json;

namespace ORIScore.ESPsim.App;

public sealed record RepositorySyncResult(bool Success, string Message, string TargetDirectory);
public sealed record RepoFileEntry(string Path, string Type, long Size, string? Sha256);
public sealed record RepoPackage(string Kind, string Id, string Name, string Version, string? Description, string? Category, string? Manifest, string? File, List<RepoFileEntry> Files);
public sealed record LibraryFileEntry(string Path, string Type, long Size, string? Sha256);
public sealed record LibraryPackage(string Id, string Name, string Version, string? Description, string? Category, List<LibraryFileEntry> Files);
public sealed record LibraryInstallResult(bool Success, string Message, string TargetDirectory);

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

    public async Task<LibraryInstallResult> InstallRepositoryItemsAsync(IEnumerable<RepoPackage> items, string? repositoryUrl = null, CancellationToken cancellationToken = default)
    {
        AppPaths.EnsurePortableLayout();
        string baseUrl = NormalizeBaseUrl(repositoryUrl ?? LoadConfiguredRepositoryUrl());
        using var http = new HttpClient { Timeout = TimeSpan.FromSeconds(60) };

        int files = 0;
        var errors = new List<string>();

        foreach (var item in items)
        {
            try
            {
                if (item.Kind is "boards" or "devices")
                {
                    if (string.IsNullOrWhiteSpace(item.File)) continue;
                    string sourceUrl = baseUrl + item.Kind + "/" + NormalizeRelativeFile(item.File).Replace('\\', '/');
                    string targetDir = item.Kind == "boards" ? AppPaths.BoardsDir : AppPaths.DevicesDir;
                    Directory.CreateDirectory(targetDir);
                    string outFile = Path.Combine(targetDir, Path.GetFileName(item.File));
                    byte[] data = await http.GetByteArrayAsync(sourceUrl, cancellationToken);
                    await File.WriteAllBytesAsync(outFile, data, cancellationToken);
                    files++;
                    continue;
                }

                string safeId = SafePathPart(item.Id);
                string rootDir = Path.Combine(AppPaths.LibraryInstalledDir, safeId);
                Directory.CreateDirectory(rootDir);

                string manifestName = "library.json";
                await File.WriteAllTextAsync(Path.Combine(rootDir, manifestName), JsonSerializer.Serialize(item, JsonOptions), cancellationToken);

                foreach (var f in item.Files)
                {
                    string rel = NormalizeRelativeFile(f.Path);
                    if (string.IsNullOrWhiteSpace(rel)) continue;

                    string sourceUrl = baseUrl + item.Kind + "/" + safeId + "/" + rel.Replace('\\', '/');
                    string outFile = Path.Combine(rootDir, rel.Replace('/', Path.DirectorySeparatorChar));
                    Directory.CreateDirectory(Path.GetDirectoryName(outFile)!);

                    byte[] data = await http.GetByteArrayAsync(sourceUrl, cancellationToken);
                    if (!string.IsNullOrWhiteSpace(f.Sha256))
                    {
                        string hash = Convert.ToHexString(SHA256.HashData(data)).ToLowerInvariant();
                        if (!hash.Equals(f.Sha256, StringComparison.OrdinalIgnoreCase))
                        {
                            errors.Add($"{item.Kind}/{item.Id}/{rel}: SHA256 nesedí");
                            continue;
                        }
                    }
                    await File.WriteAllBytesAsync(outFile, data, cancellationToken);
                    files++;
                }
            }
            catch (Exception ex)
            {
                errors.Add($"{item.Kind}/{item.Id}: {ex.Message}");
            }
        }

        string msg = errors.Count == 0
            ? $"Nainstalováno {files} souborů."
            : $"Nainstalováno {files} souborů, chyby: {string.Join(" | ", errors)}";

        return new LibraryInstallResult(errors.Count == 0, msg, AppPaths.BaseDir);
    }

    public async Task<LibraryInstallResult> InstallLibrariesAsync(IEnumerable<LibraryPackage> libraries, string? repositoryUrl = null, CancellationToken cancellationToken = default)
    {
        var items = libraries.Select(x => new RepoPackage("libraries", x.Id, x.Name, x.Version, x.Description, x.Category, $"libraries/{x.Id}/library.json", null,
            x.Files.Select(f => new RepoFileEntry(f.Path, f.Type, f.Size, f.Sha256)).ToList()));
        return await InstallRepositoryItemsAsync(items, repositoryUrl, cancellationToken);
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
        if (path.Contains("..")) return "";
        return path;
    }

    private static string SafePathPart(string value)
    {
        var chars = value.Where(c => char.IsLetterOrDigit(c) || c is '_' or '-' or '.').ToArray();
        return chars.Length == 0 ? "item" : new string(chars);
    }
}

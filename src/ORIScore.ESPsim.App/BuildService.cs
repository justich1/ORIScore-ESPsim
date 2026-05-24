using System.Diagnostics;
using System.Globalization;
using System.Text;
using System.Text.RegularExpressions;

namespace ORIScore.ESPsim.App;

public sealed record BuildResult(bool Success, string Output, string? ExePath, string BuildDirectory);

public sealed class BuildService
{
    public async Task<BuildResult> BuildInoAsync(string inoPath, CancellationToken cancellationToken = default)
    {
        if (string.IsNullOrWhiteSpace(inoPath) || !File.Exists(inoPath))
            return new BuildResult(false, "INO soubor neexistuje.", null, "");

        AppPaths.EnsurePortableLayout();

        string installedLibs = AppPaths.LibraryInstalledDir;

        if (!Directory.Exists(installedLibs) || !Directory.GetDirectories(installedLibs).Any())
            return new BuildResult(false, "Nejsou stažené žádné knihovny. Otevři Správce repozitáře a stáhni minimálně knihovnu [arduino].", null, "");

        string arduinoHeader = Directory.GetFiles(installedLibs, "Arduino.h", SearchOption.AllDirectories).FirstOrDefault() ?? "";
        if (string.IsNullOrWhiteSpace(arduinoHeader))
            return new BuildResult(false, "Knihovna [arduino] nebyla nalezena v libraries/installed. Simulátor smí používat jen knihovny stažené z repa.", null, "");

        string buildDir = AppPaths.CurrentBuildDir;

        if (Directory.Exists(buildDir))
            Directory.Delete(buildDir, recursive: true);

        Directory.CreateDirectory(buildDir);

        string ino = await File.ReadAllTextAsync(inoPath, Encoding.UTF8, cancellationToken);
        string firmwareCpp = Path.Combine(buildDir, "GeneratedFirmware.cpp");
        string mainCpp = Path.Combine(buildDir, "FirmwareSimMain.cpp");
        string buildBat = Path.Combine(buildDir, "build.bat");
        string exePath = Path.Combine(buildDir, "FirmwareSim.exe");

        await File.WriteAllTextAsync(firmwareCpp, GenerateFirmwareCpp(ino), Encoding.UTF8, cancellationToken);
        await File.WriteAllTextAsync(mainCpp, GenerateMainCpp(), Encoding.UTF8, cancellationToken);

        string vcvars = FindVcVarsBat();
        if (string.IsNullOrWhiteSpace(vcvars))
        {
            string msg = "Nenašel jsem MSVC vcvars64.bat. Nainstaluj workload 'Desktop development with C++' ve Visual Studiu.\r\n" +
                         "Hledané cesty: Visual Studio 2022/2019 Community/Professional/Enterprise/BuildTools.";
            return new BuildResult(false, msg, null, buildDir);
        }

        var includeDirs = new List<string>();
        var librarySources = new List<string>();

        foreach (string dir in Directory.GetDirectories(installedLibs))
        {
            includeDirs.Add(dir);

            string include = Path.Combine(dir, "include");
            if (Directory.Exists(include)) includeDirs.Add(include);

            string src = Path.Combine(dir, "src");
            if (Directory.Exists(src)) includeDirs.Add(src);

            // Některé stuby mají .h/.cpp přímo v kořeni, jiné v include/src.
            librarySources.AddRange(Directory.GetFiles(dir, "*.cpp", SearchOption.AllDirectories));
        }

        string includeArgs = string.Join(" ", includeDirs.Distinct(StringComparer.OrdinalIgnoreCase).Select(d => "/I" + Quote(d)));
        string sourceArgs = Quote(mainCpp) + " " + Quote(firmwareCpp) + " " + string.Join(" ", librarySources.Distinct(StringComparer.OrdinalIgnoreCase).Select(Quote));

        int buildCodePage = GetBuildLogCodePage();

        string bat = $"""
@echo off
setlocal
set VSLANG=1029
call "{vcvars}" >nul
chcp {buildCodePage} >nul
cl /nologo /std:c++17 /EHsc /utf-8 /DORISIM {includeArgs} {sourceArgs} /Fe:"{exePath}" /link Ws2_32.lib
exit /b %ERRORLEVEL%
""";
        await File.WriteAllTextAsync(buildBat, bat, Encoding.ASCII, cancellationToken);

        var output = new StringBuilder();
        output.AppendLine("Build directory: " + buildDir);
        output.AppendLine("INO: " + inoPath);
        output.AppendLine("Compiler env: " + vcvars);
        output.AppendLine();

        var psi = new ProcessStartInfo
        {
            FileName = "cmd.exe",
            Arguments = "/c \"" + buildBat + "\"",
            WorkingDirectory = buildDir,
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = true,
            StandardOutputEncoding = GetBuildLogEncoding(),
            StandardErrorEncoding = GetBuildLogEncoding()
        };

        using var p = new Process { StartInfo = psi };
        p.OutputDataReceived += (_, e) => { if (e.Data != null) lock (output) output.AppendLine(e.Data); };
        p.ErrorDataReceived += (_, e) => { if (e.Data != null) lock (output) output.AppendLine(e.Data); };

        p.Start();
        p.BeginOutputReadLine();
        p.BeginErrorReadLine();
        await p.WaitForExitAsync(cancellationToken);

        bool success = p.ExitCode == 0 && File.Exists(exePath);
        output.AppendLine();
        output.AppendLine(success ? "BUILD OK" : "BUILD FAILED");

        return new BuildResult(success, output.ToString(), success ? exePath : null, buildDir);
    }

    private static string GenerateFirmwareCpp(string ino)
    {
        var sb = new StringBuilder();

        sb.AppendLine("#include \"Arduino.h\"");
        sb.AppendLine();

        sb.AppendLine("// ---- User INO content with generated prototypes ----");
        sb.AppendLine(InsertPrototypesIntoIno(ino));

        return sb.ToString();
    }

    private static string InsertPrototypesIntoIno(string ino)
    {
        var prototypes = GenerateFunctionPrototypes(ino)
            .Distinct()
            .OrderBy(x => x)
            .ToList();

        if (prototypes.Count == 0)
            return ino;

        int insertAt = FindFirstFunctionDefinitionIndex(ino);

        if (insertAt < 0)
        {
            return string.Join(Environment.NewLine, prototypes) +
                   Environment.NewLine +
                   Environment.NewLine +
                   ino;
        }

        var sb = new StringBuilder();

        sb.Append(ino[..insertAt]);
        sb.AppendLine();
        sb.AppendLine("// ---- ORIScore ESPsim generated prototypes ----");

        foreach (string p in prototypes)
            sb.AppendLine(p);

        sb.AppendLine("// ---- End generated prototypes ----");
        sb.AppendLine();

        sb.Append(ino[insertAt..]);

        return sb.ToString();
    }

    private static int FindFirstFunctionDefinitionIndex(string code)
    {
        // Najde první běžnou globální funkci.
        // U tvého INO je to až po struct/enum/global proměnných,
        // takže prototypy se vloží až za definice typů.
        var rx = new Regex(
            @"(?m)^\s*(?:void|bool|int|uint8_t|uint16_t|uint32_t|int16_t|int32_t|unsigned\s+long|long|float|double|String|size_t|char|const\s+char\*)\s+[A-Za-z_]\w*\s*\([^;{}]*\)\s*\{",
            RegexOptions.Multiline
        );

        var m = rx.Match(code);
        return m.Success ? m.Index : -1;
    }

    private static string StripDefaultArguments(string signature)
    {
        // C++ nesmí mít default argument v prototypu i v definici současně.
        // Např.:
        // bool sendAT(const String& cmd, String* response = nullptr, uint32_t timeoutMs = 3000)
        // změní na:
        // bool sendAT(const String& cmd, String* response, uint32_t timeoutMs)

        int open = signature.IndexOf('(');
        int close = signature.LastIndexOf(')');

        if (open < 0 || close < open)
            return signature;

        string before = signature[..(open + 1)];
        string args = signature[(open + 1)..close];
        string after = signature[close..];

        var parts = args.Split(',');

        for (int i = 0; i < parts.Length; i++)
        {
            int eq = parts[i].IndexOf('=');
            if (eq >= 0)
                parts[i] = parts[i][..eq].Trim();
            else
                parts[i] = parts[i].Trim();
        }

        return before + string.Join(", ", parts) + after;
    }

    private static IEnumerable<string> GenerateFunctionPrototypes(string code)
    {
        // Záměrně jednoduchý parser. Stačí pro první build pokusy; podle chyb ho budeme rozšiřovat.
        string noComments = Regex.Replace(code, @"/\*.*?\*/", "", RegexOptions.Singleline);
        noComments = Regex.Replace(noComments, @"//.*", "");

        var rx = new Regex(@"(?m)^\s*((?:void|bool|int|uint8_t|uint16_t|uint32_t|int16_t|int32_t|unsigned\s+long|long|float|double|String|size_t|char|const\s+char\*)\s+[A-Za-z_]\w*\s*\([^;{}]*\))\s*\{", RegexOptions.Multiline);
        var set = new HashSet<string>();

        foreach (Match m in rx.Matches(noComments))
        {
            string signature = Regex.Replace(m.Groups[1].Value.Trim(), @"\s+", " ");
            string namePart = signature;
            int paren = namePart.IndexOf('(');
            if (paren < 0) continue;

            string beforeParen = namePart[..paren].Trim();
            string name = beforeParen.Split(' ', StringSplitOptions.RemoveEmptyEntries).LastOrDefault() ?? "";
            if (name is "if" or "for" or "while" or "switch") continue;

            signature = StripDefaultArguments(signature);
            set.Add(signature + ";");
        }

        // Arduino je sice zná až níže, ale main je potřebuje vždy.
        set.Add("void setup();");
        set.Add("void loop();");

        return set.OrderBy(x => x);
    }

    private static string GenerateMainCpp()
    {
        return """
#include "Arduino.h"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

void setup();
void loop();

static std::atomic<bool> gRunning{true};
static std::mutex gSerialFlushMutex;
static unsigned long gRamTotalBytes = 81920;

static unsigned long readEnvUnsignedLong(const char* name, unsigned long fallback) {
  const char* value = std::getenv(name);
  if (!value || !*value) return fallback;

  char* end = nullptr;
  unsigned long parsed = std::strtoul(value, &end, 10);
  if (end == value || parsed == 0) return fallback;
  return parsed;
}

static void emitRamState() {
  unsigned long total = gRamTotalBytes > 0 ? gRamTotalBytes : 81920;
  unsigned long freeHeap = ESP.getFreeHeap();
  unsigned long maxBlock = ESP.getMaxFreeBlockSize();

  if (freeHeap > total) total = freeHeap;
  if (maxBlock > freeHeap) maxBlock = freeHeap;

  std::cout << "RAMSTATE TOTAL=" << total
            << " FREE=" << freeHeap
            << " MAX=" << maxBlock
            << std::endl;
}

static void ramMonitorThread() {
  while (gRunning) {
    emitRamState();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

static std::vector<std::string> splitPipeEscaped(const std::string& s) {
  std::vector<std::string> out;
  std::string cur;
  bool esc = false;

  for (char c : s) {
    if (esc) {
      if (c == 'p') cur.push_back('|');
      else if (c == '\\') cur.push_back('\\');
      else {
        cur.push_back('\\');
        cur.push_back(c);
      }

      esc = false;
      continue;
    }

    if (c == '\\') {
      esc = true;
      continue;
    }

    if (c == '|') {
      out.push_back(cur);
      cur.clear();
      continue;
    }

    cur.push_back(c);
  }

  if (esc) cur.push_back('\\');
  out.push_back(cur);
  return out;
}

static std::string trimStd(std::string s) {
  while (!s.empty() && (
    s.front() == ' ' ||
    s.front() == '\t' ||
    s.front() == '\r' ||
    s.front() == '\n'
  )) {
    s.erase(s.begin());
  }

  while (!s.empty() && (
    s.back() == ' ' ||
    s.back() == '\t' ||
    s.back() == '\r' ||
    s.back() == '\n'
  )) {
    s.pop_back();
  }

  return s;
}

static std::string upperStd(std::string s) {
  for (char& c : s) {
    if (c >= 'a' && c <= 'z') {
      c = (char)(c - 'a' + 'A');
    }
  }

  return s;
}

static std::string normalizeModemPayload(std::string payload) {
  payload = trimStd(payload);
  std::string up = upperStd(payload);

  // Běžné ruční odpovědi piš klidně malými písmeny.
  // Hex/data/downlinky a složitější odpovědi necháváme beze změny.
  if (up == "OK") return "OK";
  if (up == "ERROR") return "ERROR";
  if (up == "JOINED") return "JOINED";
  if (up == "JOIN_FAILED") return "JOIN_FAILED";
  if (up == "JOIN FAILED") return "JOIN FAILED";
  if (up == "TX_DONE") return "TX_DONE";
  if (up == "SEND OK") return "SEND OK";
  if (up == "SEND DONE") return "SEND DONE";
  if (up == "DONE") return "DONE";

  return payload;
}

static void injectModemRx(const std::string& rawPayload) {
  std::string payload = normalizeModemPayload(rawPayload);
  String data(payload + "\r\n");

  // Důležité:
  // Musí existovat jen JEDNA cesta dovnitř.
  // simInjectSerialRxAll() už má nakrmit všechny registrované porty
  // včetně Serial / Serial1 / SoftwareSerial podle FakeESP knihovny.
  //
  // Kdyby registry z nějakého důvodu nebyl dostupný, použije se fallback.
  int count = simGetSerialPortCount();

  if (count > 0) {
    simInjectSerialRxAll(data);
  }
  else {
    Serial.injectRx(data);
    Serial1.injectRx(data);
  }

  std::cout << "SERIAL < " << payload << std::endl;
}

static void printTxLines(const std::string& label, const String& tx) {
  if (!tx.length()) return;

  std::string s = tx.v;
  std::string line;

  for (char c : s) {
    if (c == '\r') continue;

    if (c == '\n') {
      if (!line.empty()) {
        std::cout << label << " > " << line << std::endl;
        line.clear();
      }
      continue;
    }

    line.push_back(c);
  }

  // Vypiš i neukončený kus hned.
  // Některé knihovny dávají print("AT") a CR/LF až dalším printem.
  if (!line.empty()) {
    std::cout << label << " > " << line << std::endl;
  }
}

static void flushSerialTx() {
  std::lock_guard<std::mutex> lock(gSerialFlushMutex);

  // Primárně čteme registry všech FakeSerial portů.
  // To řeší Serial, Serial1, SoftwareSerial i další instance,
  // pokud je FakeESP knihovna správně registruje.
  int count = simGetSerialPortCount();

  for (int i = 0; i < count; i++) {
    String tx = simTakeSerialTxByIndex(i);
    if (!tx.length()) continue;

    std::string label = "ESP";
    if (i > 0) label += std::to_string(i);

    printTxLines(label, tx);
  }

  // Fallback:
  // Když by registry náhodou neobsahoval globální Serial/Serial1,
  // tady se ještě vyčte zbytek.
  // Pokud je registry obsahoval, takeTxLog() už bude prázdné,
  // takže se nic nevypíše dvakrát.
  printTxLines("ESP", Serial.takeTxLog());
  printTxLines("ESP1", Serial1.takeTxLog());
}

static void serialFlushThread() {
  while (gRunning) {
    flushSerialTx();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
}

static void stdinThread() {
  std::string line;

  while (gRunning && std::getline(std::cin, line)) {
    if (line == "EXIT") {
      gRunning = false;
      break;
    }

    if (line.rfind("MODEM ", 0) == 0) {
      injectModemRx(line.substr(6));
      continue;
    }

    if (line == "HW CLEAR") {
      simVirtualHwClear();
      std::cout << "HWSTATE CLEAR" << std::endl;
      continue;
    }

    if (line.rfind("HW SET|", 0) == 0) {
      // Format: HW SET|type|name|pin|address|value|humidity|pressure|connected
      std::vector<std::string> parts = splitPipeEscaped(line.substr(7));
      while (parts.size() < 9) parts.push_back("");

      bool connected = !(
        parts[8] == "0" ||
        parts[8] == "false" ||
        parts[8] == "FALSE"
      );

      simVirtualHwSet(
        String(parts[0]),
        String(parts[1]),
        String(parts[2]),
        String(parts[3]),
        String(parts[4]),
        String(parts[5]),
        String(parts[6]),
        connected
      );

      std::cout << "HWSTATE SET TYPE=" << parts[0]
                << " NAME=" << parts[1]
                << " PIN=" << parts[2]
                << " CONNECTED=" << (connected ? 1 : 0)
                << std::endl;
      continue;
    }

    if (line.rfind("PIN ", 0) == 0) {
      // Format: PIN <gpio> <HIGH|LOW|1|0>
      std::string rest = line.substr(4);
      std::istringstream ss(rest);

      int gpio = -1;
      std::string valueText;
      ss >> gpio >> valueText;

      if (gpio >= 0 && gpio <= 255) {
        int value = LOW;

        if (
          valueText == "HIGH" ||
          valueText == "high" ||
          valueText == "1"
        ) {
          value = HIGH;
        }

        simSetDigitalInput((uint8_t)gpio, (uint8_t)value);
        std::cout << "SIM PIN GPIO=" << gpio << " VALUE=" << value << std::endl;
      }
      else {
        std::cout << "WARN bad PIN command: " << line << std::endl;
      }

      continue;
    }

    if (line.rfind("TIMESCALE ", 0) == 0) {
      std::string value = line.substr(10);
      double scale = std::atof(value.c_str());
      simSetTimeScale(scale);
      std::cout << "SIM TIMESCALE " << scale << std::endl;
      continue;
    }

    if (line == "RESET") {
      simSetMillis(0);
      ESP.resetHeap((uint32_t)gRamTotalBytes, (uint32_t)gRamTotalBytes);
      emitRamState();

      // Před resetem vyplivni co ještě zůstalo v TX.
      flushSerialTx();

      Serial.clearTxLog();
      Serial1.clearTxLog();

      simVirtualHwClear();

      std::cout << "SIM reset" << std::endl;
      continue;
    }

    // Ruční konzole:
    // Když pošleš jen OK bez prefixu, ber to jako odpověď modemu.
    // Tím zůstane kompatibilní staré ruční testování.
    injectModemRx(line);
  }
}

int main() {
  std::cout << "ORIScore ESPsim FirmwareSim starting" << std::endl;

  gRamTotalBytes = readEnvUnsignedLong("ORISIM_RAM_TOTAL", 81920);

  // Nastav kapacitu simulovaného ESP heapu podle vybrané desky.
  // Board JSON určuje jen kapacitu; aktuální obsazení dodá FakeESP runtime.
  ESP.resetHeap((uint32_t)gRamTotalBytes, (uint32_t)gRamTotalBytes);
  emitRamState();

  std::thread input(stdinThread);
  std::thread serialOut(serialFlushThread);
  std::thread ramOut(ramMonitorThread);

  try {
    setup();

    while (gRunning) {
      loop();
      flushSerialTx();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  catch (const std::exception& ex) {
    std::cout << "FIRMWARE EXCEPTION: " << ex.what() << std::endl;
  }
  catch (...) {
    std::cout << "FIRMWARE UNKNOWN EXCEPTION" << std::endl;
  }

  gRunning = false;

  if (input.joinable()) input.detach();
  if (serialOut.joinable()) serialOut.join();
  if (ramOut.joinable()) ramOut.join();

  emitRamState();
  flushSerialTx();

  std::cout << "ORIScore ESPsim FirmwareSim stopped" << std::endl;
  return 0;
}
""";
    }

    private static int GetBuildLogCodePage()
    {
        try
        {
            int cp = CultureInfo.CurrentCulture.TextInfo.OEMCodePage;
            return cp > 0 ? cp : 65001;
        }
        catch
        {
            return 65001;
        }
    }

    private static Encoding GetBuildLogEncoding()
    {
        try
        {
            return Encoding.GetEncoding(GetBuildLogCodePage());
        }
        catch
        {
            return Encoding.UTF8;
        }
    }

    private static string FindVcVarsBat()
    {
        // 1) Primárně použij vswhere.exe - najde VS 2019/2022/2026/BuildTools automaticky
        string pfx86 = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86);

        string vswhere = Path.Combine(
            pfx86,
            "Microsoft Visual Studio",
            "Installer",
            "vswhere.exe"
        );

        if (File.Exists(vswhere))
        {
            try
            {
                var psi = new ProcessStartInfo
                {
                    FileName = vswhere,
                    Arguments = "-latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath",
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    CreateNoWindow = true
                };

                using var p = Process.Start(psi);

                if (p != null)
                {
                    string installPath = p.StandardOutput.ReadToEnd().Trim();
                    p.WaitForExit(5000);

                    if (!string.IsNullOrWhiteSpace(installPath))
                    {
                        string vcvars = Path.Combine(
                            installPath,
                            "VC",
                            "Auxiliary",
                            "Build",
                            "vcvars64.bat"
                        );

                        if (File.Exists(vcvars))
                            return vcvars;
                    }
                }
            }
            catch
            {
                // fallback níže
            }
        }

        // 2) Fallback - projde všechny roky a edice ve složce Microsoft Visual Studio
        string pf = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles);

        var candidates = new List<string>();

        foreach (string baseDir in new[] { pf, pfx86 }.Where(Directory.Exists))
        {
            string vsRoot = Path.Combine(baseDir, "Microsoft Visual Studio");

            if (!Directory.Exists(vsRoot))
                continue;

            foreach (string yearDir in Directory.GetDirectories(vsRoot))
            {
                foreach (string editionDir in Directory.GetDirectories(yearDir))
                {
                    candidates.Add(
                        Path.Combine(
                            editionDir,
                            "VC",
                            "Auxiliary",
                            "Build",
                            "vcvars64.bat"
                        )
                    );
                }
            }
        }

        string? env = Environment.GetEnvironmentVariable("VSINSTALLDIR");

        if (!string.IsNullOrWhiteSpace(env))
        {
            candidates.Insert(
                0,
                Path.Combine(env, "VC", "Auxiliary", "Build", "vcvars64.bat")
            );
        }

        return candidates.FirstOrDefault(File.Exists) ?? "";
    }

    private static string Quote(string s) => "\"" + s + "\"";
}

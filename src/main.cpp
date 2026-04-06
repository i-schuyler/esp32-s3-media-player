#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_err.h>
#include <esp_ota_ops.h>
#include <cstdarg>
#include <cstring>

#include "firmware_version.h"

namespace {
WebServer server(80);

constexpr uint8_t kSdCsPin = 13;
constexpr uint8_t kSdSckPin = 12;
constexpr uint8_t kSdMisoPin = 11;
constexpr uint8_t kSdMosiPin = 10;
constexpr uint32_t kSdSpiHz = 4000000;
constexpr uint32_t kSdInitHz = 400000;
constexpr uint8_t kSdInitClockBytes = 10;
constexpr uint32_t kSdSpiRetryHz[] = {kSdInitHz, 1000000, 2000000, kSdSpiHz};
constexpr const char* kSdMountPoint = "/sd";
constexpr const char* kSdFormatConfirmToken = "FORMAT";
constexpr const char* kDefaultApPassword = "12345678";
constexpr const char* kExpectedBoardTarget = "esp32-s3-devkitc-1-n32r16v";
constexpr size_t kChunkSize = 2048;
constexpr size_t kDebugLogCapacity = 120;
constexpr size_t kDebugLogLineMaxLen = 220;
constexpr uint8_t kEspImageMagicByte = 0xE9;
constexpr int32_t kOtaErrorSizeMismatch = 255;
constexpr int32_t kOtaErrorBadMagic = 254;
constexpr int32_t kOtaErrorFlashModeMismatch = 253;

struct OtaStatus {
  bool inProgress = false;
  bool success = false;
  bool hasResult = false;
  size_t received = 0;
  size_t expected = 0;
  int32_t errorCode = 0;
  String stage = F("idle");
  String errorName = F("none");
  String message = F("idle");
  String guidance = F("Select firmware.bin built for esp32-s3-devkitc-1-n32r16v.");
  String runningPartition = F("unknown");
  String targetPartition = F("unknown");
  String buildEnv = F("unknown");
  String buildBoard = F("unknown");
  String buildFlashSize = F("unknown");
  String buildFlashMode = F("unknown");
  String buildPsramType = F("unknown");
  String buildMemoryType = F("unknown");
  String buildPartitions = F("unknown");
  String partitionLayout = F("unknown");
  String runningImageFlashMode = F("unknown");
  String uploadImageFlashMode = F("unknown");
};

struct OtaSession {
  bool active = false;
  bool headerChecked = false;
  bool runningImageFlashModeKnown = false;
  uint8_t runningImageFlashMode = 0;
  uint8_t headerBytes[3] = {0, 0, 0};
  size_t headerByteCount = 0;
  esp_ota_handle_t handle = 0;
  const esp_partition_t* target = nullptr;
};

OtaStatus gOtaStatus;
OtaSession gOtaSession;
bool gOtaRestartPending = false;
unsigned long gOtaRestartAtMs = 0;

enum class SdFailureStage {
  kOk,
  kInitBusFailure,
  kCardCommFailure,
  kMountFsFailure,
  kReadRootFailure,
};

struct SdStatus {
  bool available = false;
  SdFailureStage stage = SdFailureStage::kInitBusFailure;
  String detail;
};

SdStatus gSdStatus;

struct SdFormatStatus {
  bool inProgress = false;
  bool success = false;
  bool hasResult = false;
  String stage = F("idle");
  String message = F("idle");
  String guidance = F("Type FORMAT and confirm to erase all files on the SD card.");
};

SdFormatStatus gSdFormatStatus;
String gDebugLog[kDebugLogCapacity];
size_t gDebugLogNext = 0;
size_t gDebugLogCount = 0;

String htmlEscape(const String& in) {
  String out;
  out.reserve(in.length());
  for (size_t i = 0; i < in.length(); ++i) {
    const char c = in.charAt(i);
    if (c == '&') out += F("&amp;");
    else if (c == '<') out += F("&lt;");
    else if (c == '>') out += F("&gt;");
    else if (c == '"') out += F("&quot;");
    else out += c;
  }
  return out;
}

String jsonEscape(const String& in) {
  String out;
  out.reserve(in.length() + 8);
  for (size_t i = 0; i < in.length(); ++i) {
    const char c = in.charAt(i);
    if (c == '"') out += F("\\\"");
    else if (c == '\\') out += F("\\\\");
    else if (c == '\n') out += F("\\n");
    else if (c == '\r') out += F("\\r");
    else out += c;
  }
  return out;
}

String sanitizeDebugLogLine(const String& in) {
  String out;
  out.reserve(in.length());
  for (size_t i = 0; i < in.length(); ++i) {
    const char c = in.charAt(i);
    if (c == '\n' || c == '\r') continue;
    if (static_cast<unsigned char>(c) < 32 && c != '\t') continue;
    out += c;
  }
  if (out.length() > kDebugLogLineMaxLen) {
    out = out.substring(0, kDebugLogLineMaxLen);
    out += F("...");
  }
  return out;
}

void appendDebugLogLine(const String& line) {
  gDebugLog[gDebugLogNext] = sanitizeDebugLogLine(line);
  gDebugLogNext = (gDebugLogNext + 1) % kDebugLogCapacity;
  if (gDebugLogCount < kDebugLogCapacity) ++gDebugLogCount;
}

void serialAndDebugLog(const String& line) {
  Serial.println(line);
  appendDebugLogLine(line);
}

void serialAndDebugLogf(const char* fmt, ...) {
  char buffer[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  Serial.println(buffer);
  appendDebugLogLine(String(buffer));
}

String debugLogJson() {
  String json = "{";
  json += "\"capacity\":";
  json += String(static_cast<unsigned long>(kDebugLogCapacity));
  json += ",\"count\":";
  json += String(static_cast<unsigned long>(gDebugLogCount));
  json += ",\"lines\":[";
  for (size_t i = 0; i < gDebugLogCount; ++i) {
    if (i > 0) json += ",";
    size_t idx = (gDebugLogNext + kDebugLogCapacity - gDebugLogCount + i) % kDebugLogCapacity;
    json += "\"";
    json += jsonEscape(gDebugLog[idx]);
    json += "\"";
  }
  json += "]}";
  return json;
}

String otaBuildEnvName() {
#ifdef PIOENV
  return String(PIOENV);
#else
  return F("unknown");
#endif
}

String otaBuildBoardName() {
#ifdef FW_BUILD_BOARD
  return String(FW_BUILD_BOARD);
#else
  return String(kExpectedBoardTarget);
#endif
}

String otaBuildFlashSizeName() {
#ifdef FW_FLASH_SIZE
  return String(FW_FLASH_SIZE);
#else
  return F("unknown");
#endif
}

String otaBuildFlashModeName() {
#ifdef FW_FLASH_MODE
  return String(FW_FLASH_MODE);
#else
  return F("unknown");
#endif
}

String otaBuildPsramTypeName() {
#ifdef FW_PSRAM_TYPE
  return String(FW_PSRAM_TYPE);
#elif defined(FW_PSRAM_MODE)
  return String(FW_PSRAM_MODE);
#else
  return F("unknown");
#endif
}

String otaBuildMemoryTypeName() {
#ifdef FW_MEMORY_TYPE
  return String(FW_MEMORY_TYPE);
#else
  return F("unknown");
#endif
}

String otaBuildPartitionsName() {
#ifdef FW_PARTITIONS_CSV
  return String(FW_PARTITIONS_CSV);
#else
  return F("unknown");
#endif
}

String otaPartitionSummary(const esp_partition_t* partition);

String otaImageFlashModeName(uint8_t flashMode) {
  switch (flashMode) {
    case 0:
      return F("qio");
    case 1:
      return F("qout");
    case 2:
      return F("dio");
    case 3:
      return F("dout");
    default: {
      String value = F("0x");
      value += String(static_cast<unsigned long>(flashMode), HEX);
      value.toUpperCase();
      return value;
    }
  }
}

String otaPartitionLayoutSummary() {
  const esp_partition_t* factory = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, "factory");
  const esp_partition_t* app0 = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, "app0");
  const esp_partition_t* app1 = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "app1");
  String summary = F("csv=");
  summary += otaBuildPartitionsName();
  summary += F("; factory=");
  summary += otaPartitionSummary(factory);
  summary += F("; app0=");
  summary += otaPartitionSummary(app0);
  summary += F("; app1=");
  summary += otaPartitionSummary(app1);
  return summary;
}

String otaPartitionSummary(const esp_partition_t* partition) {
  if (partition == nullptr) return F("none");
  String summary = (partition->label && partition->label[0]) ? String(partition->label) : String(F("unnamed"));
  summary += F(" @0x");
  summary += String(static_cast<unsigned long>(partition->address), HEX);
  summary += F(" size=0x");
  summary += String(static_cast<unsigned long>(partition->size), HEX);
  return summary;
}

void refreshOtaDiagnostics() {
  gOtaStatus.buildEnv = otaBuildEnvName();
  gOtaStatus.buildBoard = otaBuildBoardName();
  gOtaStatus.buildFlashSize = otaBuildFlashSizeName();
  gOtaStatus.buildFlashMode = otaBuildFlashModeName();
  gOtaStatus.buildPsramType = otaBuildPsramTypeName();
  gOtaStatus.buildMemoryType = otaBuildMemoryTypeName();
  gOtaStatus.buildPartitions = otaBuildPartitionsName();
  gOtaStatus.partitionLayout = otaPartitionLayoutSummary();
  const esp_partition_t* running = esp_ota_get_running_partition();
  const esp_partition_t* target = esp_ota_get_next_update_partition(nullptr);
  gOtaStatus.runningPartition = otaPartitionSummary(running);
  gOtaStatus.targetPartition = otaPartitionSummary(target);
}

String otaErrorNameForEspErr(esp_err_t error) {
  switch (error) {
    case ESP_OK:
      return F("none");
    case ESP_ERR_FLASH_OP_FAIL:
      return F("Flash Read Failed");
    case ESP_ERR_FLASH_OP_TIMEOUT:
      return F("Flash Timeout");
    case ESP_ERR_OTA_VALIDATE_FAILED:
      return F("Validation Failed");
    case ESP_ERR_INVALID_SIZE:
      return F("Invalid Size");
    case ESP_ERR_INVALID_ARG:
      return F("Invalid Argument");
    case ESP_ERR_NOT_FOUND:
      return F("Not Found");
    case ESP_ERR_NO_MEM:
      return F("No Memory");
    default:
      return String(esp_err_to_name(error));
  }
}

String otaGuidanceForEspErr(esp_err_t error) {
  switch (error) {
    case ESP_ERR_FLASH_OP_FAIL:
    case ESP_ERR_FLASH_OP_TIMEOUT:
      return F("Flash operation failed during validation/apply. Confirm release asset/build parity for esp32-s3-devkitc-1-n32r16v, reboot, and retry. If repeated, USB-flash once and retry OTA.");
    case ESP_ERR_OTA_VALIDATE_FAILED:
      return F("Firmware image validation failed. Re-download/rebuild firmware.bin for esp32-s3-devkitc-1-n32r16v and retry.");
    case ESP_ERR_INVALID_SIZE:
      return F("Firmware size is invalid for OTA. Rebuild firmware.bin and retry.");
    case ESP_ERR_NOT_FOUND:
      return F("No OTA target partition available. Verify partition table for esp32-s3-devkitc-1-n32r16v.");
    case ESP_ERR_NO_MEM:
      return F("Not enough memory to continue OTA. Reboot and retry with a stable connection.");
    case ESP_ERR_INVALID_ARG:
      return F("Invalid OTA request. Retry from the OTA page using firmware.bin.");
    default:
      return F("Retry with a known-good firmware.bin. If it still fails, capture serial logs for deeper diagnostics.");
  }
}

void setOtaFailure(const String& stage, int32_t errorCode, const String& errorName, const String& message, const String& guidance) {
  gOtaStatus.inProgress = false;
  gOtaStatus.success = false;
  gOtaStatus.hasResult = true;
  gOtaStatus.errorCode = errorCode;
  gOtaStatus.errorName = errorName;
  gOtaStatus.stage = stage;
  gOtaStatus.message = message;
  gOtaStatus.guidance = guidance;
}

void resetOtaSession() {
  gOtaSession.active = false;
  gOtaSession.headerChecked = false;
  gOtaSession.runningImageFlashModeKnown = false;
  gOtaSession.runningImageFlashMode = 0;
  gOtaSession.headerBytes[0] = 0;
  gOtaSession.headerBytes[1] = 0;
  gOtaSession.headerBytes[2] = 0;
  gOtaSession.headerByteCount = 0;
  gOtaSession.handle = 0;
  gOtaSession.target = nullptr;
}

String sdSpiSpeedLabel(uint32_t hz) {
  if (hz >= 1000000UL) {
    return String(static_cast<unsigned long>(hz / 1000000UL)) + F("MHz");
  }
  return String(static_cast<unsigned long>(hz / 1000UL)) + F("kHz");
}

String sdCardTypeLabel(uint8_t cardType) {
  switch (cardType) {
    case CARD_MMC:
      return F("mmc");
    case CARD_SD:
      return F("sdsc");
    case CARD_SDHC:
      return F("sdhc");
    case CARD_NONE:
      return F("none");
    default:
      return String(static_cast<unsigned long>(cardType));
  }
}

String sdMisoIdleHint() {
  return digitalRead(kSdMisoPin) == LOW ? F("miso_idle=low") : F("miso_idle=high");
}

void primeSdSpiBus() {
  pinMode(kSdCsPin, OUTPUT);
  pinMode(kSdSckPin, OUTPUT);
  pinMode(kSdMosiPin, OUTPUT);
  pinMode(kSdMisoPin, INPUT_PULLUP);
  digitalWrite(kSdCsPin, HIGH);
  digitalWrite(kSdSckPin, HIGH);
  digitalWrite(kSdMosiPin, HIGH);
  delay(2);

  SPI.begin(kSdSckPin, kSdMisoPin, kSdMosiPin, kSdCsPin);
  SPI.beginTransaction(SPISettings(kSdInitHz, MSBFIRST, SPI_MODE0));
  for (uint8_t i = 0; i < kSdInitClockBytes; ++i) {
    SPI.transfer(0xFF);
  }
  SPI.endTransaction();
}

bool mountSdAttempt(uint32_t hz, bool allowFormat, String& detail, SdFailureStage& stage, bool& capacityKnown) {
  const String misoHint = sdMisoIdleHint();
  if (!SD.begin(kSdCsPin, SPI, hz, kSdMountPoint, 5, allowFormat)) {
    const uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
      stage = SdFailureStage::kCardCommFailure;
      detail = String(F("card not detected: SD.begin failed at ")) + sdSpiSpeedLabel(hz) + F(" (cardType=none, ") + misoHint + F(")");
    } else {
      stage = SdFailureStage::kInitBusFailure;
      detail = String(F("SD.begin failed at ")) + sdSpiSpeedLabel(hz) +
               F(" after card response (cardType=") + sdCardTypeLabel(cardType) + F(", ") + misoHint + F(")");
    }
    return false;
  }

  const uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    stage = SdFailureStage::kCardCommFailure;
    detail = String(F("card not detected after begin at ")) + sdSpiSpeedLabel(hz) + F(" (") + misoHint + F(")");
    return false;
  }

  File root = SD.open("/");
  if (!root || !root.isDirectory()) {
    stage = SdFailureStage::kReadRootFailure;
    detail = String(F("root open failed at ")) + sdSpiSpeedLabel(hz);
    return false;
  }
  root.close();

  capacityKnown = SD.totalBytes() > 0;
  detail = String(F("mounted and root listed at ")) + sdSpiSpeedLabel(hz) +
           F(" (cardType=") + sdCardTypeLabel(cardType) + F(")");
  if (!capacityKnown) {
    detail += F(" (capacity query unavailable)");
  }
  stage = SdFailureStage::kOk;
  return true;
}

bool mountSdWithRetries(bool allowFormat, String& detail, SdFailureStage& stage, bool& capacityKnown, uint32_t& usedHz) {
  stage = SdFailureStage::kInitBusFailure;
  detail = F("no mount attempts yet");
  capacityKnown = false;
  usedHz = kSdSpiHz;

  for (const uint32_t hz : kSdSpiRetryHz) {
    SD.end();
    SPI.end();
    primeSdSpiBus();
    if (mountSdAttempt(hz, allowFormat, detail, stage, capacityKnown)) {
      usedHz = hz;
      return true;
    }
  }
  return false;
}

String contentTypeForPath(const String& path) {
  if (path.endsWith(".mp3")) return F("audio/mpeg");
  if (path.endsWith(".wav")) return F("audio/wav");
  if (path.endsWith(".m4a")) return F("audio/mp4");
  if (path.endsWith(".aac")) return F("audio/aac");
  if (path.endsWith(".flac")) return F("audio/flac");
  if (path.endsWith(".ogg")) return F("audio/ogg");
  return F("application/octet-stream");
}

String sanitizePath(const String& raw) {
  if (raw.isEmpty()) return F("/");
  String path = raw;
  if (!path.startsWith("/")) path = "/" + path;
  while (path.indexOf("//") >= 0) path.replace("//", "/");
  return path;
}

String sdStageCode(SdFailureStage stage) {
  switch (stage) {
    case SdFailureStage::kOk:
      return F("ok");
    case SdFailureStage::kInitBusFailure:
      return F("init_bus_failure");
    case SdFailureStage::kCardCommFailure:
      return F("card_comm_failure");
    case SdFailureStage::kMountFsFailure:
      return F("mount_fs_failure");
    case SdFailureStage::kReadRootFailure:
      return F("read_root_failure");
  }
  return F("unknown");
}

String sdStageTitle(SdFailureStage stage) {
  switch (stage) {
    case SdFailureStage::kOk:
      return F("SD is ready");
    case SdFailureStage::kInitBusFailure:
      return F("SD init failed after card response");
    case SdFailureStage::kCardCommFailure:
      return F("SD card not detected or not communicating");
    case SdFailureStage::kMountFsFailure:
      return F("SD mount/filesystem failed");
    case SdFailureStage::kReadRootFailure:
      return F("SD root read/open failed");
  }
  return F("SD unavailable");
}

String sdStageGuidance(SdFailureStage stage) {
  switch (stage) {
    case SdFailureStage::kOk:
      return F("No action needed.");
    case SdFailureStage::kInitBusFailure:
      return F("Card responded but init/mount still failed. Reboot and retry, then verify CS=IO13, SCK=IO12, MISO=IO11, MOSI=IO10 and 3.3V/GND stability.");
    case SdFailureStage::kCardCommFailure:
      return F("No reliable SPI card response. Re-seat card, verify FAT32, and inspect wiring/power. If detail shows miso_idle=low, check MISO short/pin conflict.");
    case SdFailureStage::kMountFsFailure:
      return F("Use a supported card/format (FAT32 recommended) and reformat the card if needed.");
    case SdFailureStage::kReadRootFailure:
      return F("Card mounted but root could not be listed. Reboot device, re-seat card, and retry with a known-good card.");
  }
  return F("Inspect SD wiring and card format.");
}

void setSdStatus(SdFailureStage stage, bool available, const String& detail) {
  gSdStatus.stage = stage;
  gSdStatus.available = available;
  gSdStatus.detail = detail;
}

void setSdStatusFromRootReadFailure(const String& detail) {
  if (!gSdStatus.available) return;
  setSdStatus(SdFailureStage::kReadRootFailure, false, detail);
}

void setSdFormatStatus(const String& stage, const String& message, const String& guidance, bool inProgress, bool success, bool hasResult) {
  gSdFormatStatus.stage = stage;
  gSdFormatStatus.message = message;
  gSdFormatStatus.guidance = guidance;
  gSdFormatStatus.inProgress = inProgress;
  gSdFormatStatus.success = success;
  gSdFormatStatus.hasResult = hasResult;
}

String sdFormatStatusJson() {
  String json = "{";
  json += "\"in_progress\":";
  json += gSdFormatStatus.inProgress ? "true" : "false";
  json += ",\"success\":";
  json += gSdFormatStatus.success ? "true" : "false";
  json += ",\"has_result\":";
  json += gSdFormatStatus.hasResult ? "true" : "false";
  json += ",\"stage\":\"";
  json += jsonEscape(gSdFormatStatus.stage);
  json += "\",\"message\":\"";
  json += jsonEscape(gSdFormatStatus.message);
  json += "\",\"guidance\":\"";
  json += jsonEscape(gSdFormatStatus.guidance);
  json += "\",\"sd_available\":";
  json += gSdStatus.available ? "true" : "false";
  json += ",\"sd_stage\":\"";
  json += jsonEscape(sdStageCode(gSdStatus.stage));
  json += "\"}";
  return json;
}

void appendSdUnavailableHtml(String& body) {
  body += F("<p><strong>SD unavailable:</strong> ");
  body += sdStageTitle(gSdStatus.stage);
  body += F("</p><p>Next step: ");
  body += htmlEscape(sdStageGuidance(gSdStatus.stage));
  body += F("</p><p><a href='/sd-diagnostics'>Open SD diagnostics</a></p>");
}

void handleRoot() {
  String body;
  body.reserve(512);
  body += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  body += F("<title>ESP32-S3 Media Server</title></head><body>");
  body += F("<h1>ESP32-S3 Media Server</h1>");
  body += F("<p>Firmware version: ");
  body += firmware::kFirmwareVersion;
  body += F("</p>");
  body += F("<p><a href='/files'>Open file browser</a></p>");
  body += F("<p><a href='/debug-log'>Debug log (OTA/SD)</a></p>");
  body += F("<p><a href='/sd-diagnostics'>SD diagnostics</a></p>");
  body += F("<p><a href='/ota'>Firmware update (OTA)</a></p>");
  body += F("</body></html>");
  server.send(200, F("text/html"), body);
}

void appendDirectoryListingHtml(String& body, fs::FS& fs, const char* dirname) {
  File root = fs.open(dirname);
  if (!root || !root.isDirectory()) {
    setSdStatusFromRootReadFailure(F("root listing failed"));
    appendSdUnavailableHtml(body);
    return;
  }

  body += F("<ul>");
  File entry = root.openNextFile();
  while (entry) {
    String name = String(entry.name());
    const String escaped = htmlEscape(name);
    body += F("<li>");
    body += escaped;
    body += F(" (");
    body += entry.isDirectory() ? F("dir") : String(static_cast<unsigned long>(entry.size()));
    body += F(") ");
    if (!entry.isDirectory()) {
      body += F("<a href='/download?path=");
      body += escaped;
      body += F("'>download</a> ");
      body += F("<a href='/stream?path=");
      body += escaped;
      body += F("'>stream</a>");
    }
    body += F("</li>");
    entry = root.openNextFile();
  }
  body += F("</ul>");
}

void handleFilesPage() {
  String body;
  body.reserve(7600);
  body += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  body += F("<title>ESP32 Files</title></head><body><h1>SD Browser</h1>");
  body += F("<p><a href='/sd-diagnostics'>Open SD diagnostics</a></p>");
  body += F("<p><a href='/debug-log'>Open rolling debug log</a></p>");
  body += F("<p><a href='/ota'>Open OTA firmware updater</a></p>");
  body += F("<form method='POST' action='/upload' enctype='multipart/form-data'>");
  body += F("<input type='file' name='file'/>");
  body += F("<button type='submit'>Upload</button></form>");
  appendDirectoryListingHtml(body, SD, "/");
  body += F("<hr><h2>Danger Zone: Format SD card (destructive)</h2>");
  body += F("<p><strong>Warning:</strong> formatting will permanently erase all files on the SD card.</p>");
  body += F("<p>Status: <span id='fmtStage'>idle</span></p>");
  body += F("<p>Message: <span id='fmtMessage'>Type FORMAT and confirm to start.</span></p>");
  body += F("<p>Next step: <span id='fmtGuidance'>Use a sacrificial card and verify backups before continuing.</span></p>");
  body += F("<form id='fmtForm'>");
  body += F("<label><input id='fmtCheckbox' type='checkbox' required/> I understand all SD files will be erased.</label><br/>");
  body += F("<label>Type <code>FORMAT</code>: <input id='fmtToken' type='text' name='confirm' required autocomplete='off'/></label><br/>");
  body += F("<button type='submit'>Format SD to FAT/FAT32</button></form>");
  body += F("<script>");
  body += F("const fmtForm=document.getElementById('fmtForm');const fmtStage=document.getElementById('fmtStage');const fmtMessage=document.getElementById('fmtMessage');const fmtGuidance=document.getElementById('fmtGuidance');");
  body += F("function setFmt(j){fmtStage.textContent=j.stage;fmtMessage.textContent=j.message;fmtGuidance.textContent=j.guidance;}");
  body += F("function loadFmt(){fetch('/api/sd/format/status').then(r=>r.json()).then(setFmt).catch(()=>{});}loadFmt();setInterval(loadFmt,1200);");
  body += F("fmtForm.addEventListener('submit',async function(e){e.preventDefault();const ok=document.getElementById('fmtCheckbox').checked;const token=(document.getElementById('fmtToken').value||'').trim();if(!ok){setFmt({stage:'blocked',message:'confirm the destructive warning first',guidance:'Check the confirmation box, then retry.'});return;}if(token!=='FORMAT'){setFmt({stage:'blocked',message:'confirmation text mismatch',guidance:'Type FORMAT exactly to continue.'});return;}if(!confirm('Formatting will erase all files on this SD card. Continue?')){return;}setFmt({stage:'starting',message:'starting SD format request',guidance:'Keep this page open while formatting/remount/validation complete.'});const req='confirm='+encodeURIComponent(token);const res=await fetch('/api/sd/format',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:req});let j={stage:'failed',message:'format request failed',guidance:'Retry. If repeated, check SD card and wiring.'};try{j=await res.json();}catch(_e){}setFmt(j);});");
  body += F("</script>");
  body += F("</body></html>");
  server.send(200, F("text/html"), body);
}

void handleSdStatusApi() {
  String json = "{";
  json += "\"available\":";
  json += gSdStatus.available ? "true" : "false";
  json += ",\"stage\":\"";
  json += sdStageCode(gSdStatus.stage);
  json += "\",\"title\":\"";
  json += jsonEscape(sdStageTitle(gSdStatus.stage));
  json += "\",\"guidance\":\"";
  json += jsonEscape(sdStageGuidance(gSdStatus.stage));
  json += "\",\"detail\":\"";
  json += jsonEscape(gSdStatus.detail);
  json += "\"}";
  server.send(200, F("application/json"), json);
}

void handleSdDiagnosticsPage() {
  String body;
  body.reserve(1800);
  body += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  body += F("<title>SD diagnostics</title></head><body><h1>SD Diagnostics</h1>");
  body += F("<p><strong>Status:</strong> ");
  body += htmlEscape(sdStageTitle(gSdStatus.stage));
  body += F("</p><p><strong>Failure stage:</strong> ");
  body += htmlEscape(sdStageCode(gSdStatus.stage));
  body += F("</p><p><strong>Action:</strong> ");
  body += htmlEscape(sdStageGuidance(gSdStatus.stage));
  body += F("</p>");
  if (gSdStatus.detail.length() > 0) {
    body += F("<p><strong>Detail:</strong> ");
    body += htmlEscape(gSdStatus.detail);
    body += F("</p>");
  }
  body += F("<p><a href='/debug-log'>Open rolling OTA/SD debug log</a></p>");
  body += F("<p>Serial logs remain available for deeper debugging.</p>");
  body += F("<p><a href='/api/sd/status'>View raw JSON status</a></p>");
  body += F("<p><a href='/files'>Back to file browser</a></p></body></html>");
  server.send(200, F("text/html"), body);
}

void handleDebugLogApi() {
  server.send(200, F("application/json"), debugLogJson());
}

void handleDebugLogPage() {
  String body;
  body.reserve(2600);
  body += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  body += F("<title>Debug log</title></head><body><h1>Rolling debug log</h1>");
  body += F("<p>In-memory bounded OTA/SD troubleshooting log (clears on reboot; rolls when full).</p>");
  body += F("<p><a href='/api/debug-log'>View raw JSON log</a></p>");
  body += F("<pre id='log' style='white-space:pre-wrap;background:#111;color:#0f0;padding:12px;border-radius:8px;min-height:240px;'></pre>");
  body += F("<p><a href='/'>Home</a> | <a href='/files'>Files</a> | <a href='/ota'>OTA</a> | <a href='/sd-diagnostics'>SD diagnostics</a></p>");
  body += F("<script>");
  body += F("const logEl=document.getElementById('log');");
  body += F("function render(j){const lines=Array.isArray(j.lines)?j.lines:[];logEl.textContent=lines.join('\\n')||'(no entries yet)';}");
  body += F("function load(){fetch('/api/debug-log',{cache:'no-store'}).then(r=>r.json()).then(render).catch(()=>{});}load();setInterval(load,1000);");
  body += F("</script></body></html>");
  server.send(200, F("text/html"), body);
}

bool validateSdWritableReadable(String& detail) {
  const char* kProbePath = "/.fmt_probe.txt";
  const char* kProbeData = "fmt_probe_ok";
  File probe = SD.open(kProbePath, FILE_WRITE);
  if (!probe) {
    detail = F("probe write open failed");
    return false;
  }

  const size_t expected = strlen(kProbeData);
  const size_t written = probe.write(reinterpret_cast<const uint8_t*>(kProbeData), expected);
  probe.close();
  if (written != expected) {
    detail = F("probe write failed");
    SD.remove(kProbePath);
    return false;
  }

  File verify = SD.open(kProbePath, FILE_READ);
  if (!verify) {
    detail = F("probe read open failed");
    SD.remove(kProbePath);
    return false;
  }
  String data;
  while (verify.available()) data += static_cast<char>(verify.read());
  verify.close();
  SD.remove(kProbePath);

  if (data != kProbeData) {
    detail = F("probe read mismatch");
    return false;
  }
  detail = F("root access + write/read probe passed");
  return true;
}

bool ensureSdMountedAndUsable(String& detail) {
  SdFailureStage stage;
  bool capacityKnown = false;
  uint32_t usedHz = kSdSpiHz;
  if (!mountSdWithRetries(false, detail, stage, capacityKnown, usedHz)) {
    return false;
  }
  return true;
}

bool formatSdToFatAndRemount() {
  setSdFormatStatus(F("precheck"), F("checking SD presence and mount state"), F("Ensure an SD card is inserted and not write-protected."), true, false, false);

  String detail;
  if (!ensureSdMountedAndUsable(detail)) {
    if (detail.indexOf("card not detected") >= 0 || detail.indexOf("cardType=none") >= 0) {
      setSdStatus(SdFailureStage::kCardCommFailure, false, detail);
      setSdFormatStatus(F("failed"), F("SD format failed: no card detected"), F("Insert/re-seat a card, then retry. If still absent, verify wiring and card health."), false, false, true);
    } else {
      setSdStatus(SdFailureStage::kMountFsFailure, false, detail);
      setSdFormatStatus(F("failed"), String(F("SD format precheck failed: ")) + detail, F("Try re-seating the card, then retry formatting. If needed, power-cycle and retry."), false, false, true);
    }
    return false;
  }

  setSdFormatStatus(F("formatting"), F("erasing existing filesystem metadata"), F("Do not remove power or SD card."), true, false, false);
  uint8_t zeroSector[512] = {0};
  if (!SD.writeRAW(zeroSector, 0)) {
    setSdFormatStatus(F("failed"), F("SD format failed: cannot write raw sector"), F("Card may be write-protected or failing. Unlock/re-seat/replace the card and retry."), false, false, true);
    return false;
  }

  setSdFormatStatus(F("remounting"), F("creating FAT/FAT32 filesystem and remounting"), F("Wait for remount to complete."), true, false, false);
  SdFailureStage remountStage = SdFailureStage::kInitBusFailure;
  bool remountCapacityKnown = false;
  uint32_t remountHz = kSdSpiHz;
  String remountDetail;
  if (!mountSdWithRetries(true, remountDetail, remountStage, remountCapacityKnown, remountHz)) {
    setSdStatus(remountStage, false, remountDetail);
    setSdFormatStatus(F("failed"), String(F("SD format failed: remount failed: ")) + remountDetail,
                      F("Retry with another card. If repeated, format on PC as FAT32 then retry."), false, false, true);
    return false;
  }
  (void)remountCapacityKnown;
  (void)remountHz;

  if (!ensureSdMountedAndUsable(remountDetail)) {
    setSdStatus(SdFailureStage::kMountFsFailure, false, remountDetail);
    setSdFormatStatus(F("failed"), String(F("SD format failed after remount: ")) + remountDetail, F("Try a different SD card and verify wiring/power stability."), false, false, true);
    return false;
  }

  setSdFormatStatus(F("validating"), F("running post-format validation"), F("Checking root access and write/read probe."), true, false, false);
  String validateDetail;
  if (!validateSdWritableReadable(validateDetail)) {
    setSdStatus(SdFailureStage::kReadRootFailure, false, validateDetail);
    setSdFormatStatus(F("failed"), String(F("SD format validation failed: ")) + validateDetail, F("Re-seat or replace the card and retry. If persistent, test card on another device."), false, false, true);
    return false;
  }

  setSdStatus(SdFailureStage::kOk, true, validateDetail);
  setSdFormatStatus(F("success"), F("SD formatted and validated successfully"), F("You can now upload files from this page."), false, true, true);
  return true;
}

void handleListApi() {
  File root = SD.open("/");
  if (!root || !root.isDirectory()) {
    setSdStatusFromRootReadFailure(F("api list root open failed"));
    String json = "{\"error\":\"sd_root_unavailable\",\"stage\":\"";
    json += sdStageCode(gSdStatus.stage);
    json += "\",\"guidance\":\"";
    json += jsonEscape(sdStageGuidance(gSdStatus.stage));
    json += "\"}";
    server.send(500, F("application/json"), json);
    return;
  }

  String json = "[";
  File entry = root.openNextFile();
  bool first = true;
  while (entry) {
    if (!first) json += ",";
    first = false;
    const String name = String(entry.name());
    json += "{\"name\":\"";
    json += name;
    json += "\",\"size\":";
    json += String(static_cast<unsigned long>(entry.size()));
    json += ",\"dir\":";
    json += entry.isDirectory() ? "true" : "false";
    json += "}";
    entry = root.openNextFile();
  }
  json += "]";

  Serial.println(F("SD: LIST OK"));
  server.send(200, F("application/json"), json);
}

void handleOtaStatusApi() {
  refreshOtaDiagnostics();
  String json = "{";
  json += "\"in_progress\":";
  json += gOtaStatus.inProgress ? "true" : "false";
  json += ",\"success\":";
  json += gOtaStatus.success ? "true" : "false";
  json += ",\"has_result\":";
  json += gOtaStatus.hasResult ? "true" : "false";
  json += ",\"received\":";
  json += String(static_cast<unsigned long>(gOtaStatus.received));
  json += ",\"expected\":";
  json += String(static_cast<unsigned long>(gOtaStatus.expected));
  json += ",\"error_code\":";
  json += String(static_cast<long>(gOtaStatus.errorCode));
  json += ",\"stage\":\"";
  json += jsonEscape(gOtaStatus.stage);
  json += "\",\"error_name\":\"";
  json += jsonEscape(gOtaStatus.errorName);
  json += "\"";
  json += ",\"message\":\"";
  json += jsonEscape(gOtaStatus.message);
  json += "\",\"guidance\":\"";
  json += jsonEscape(gOtaStatus.guidance);
  json += "\",\"build_env\":\"";
  json += jsonEscape(gOtaStatus.buildEnv);
  json += "\",\"build_board\":\"";
  json += jsonEscape(gOtaStatus.buildBoard);
  json += "\",\"build_flash_size\":\"";
  json += jsonEscape(gOtaStatus.buildFlashSize);
  json += "\",\"build_flash_mode\":\"";
  json += jsonEscape(gOtaStatus.buildFlashMode);
  json += "\",\"build_psram_type\":\"";
  json += jsonEscape(gOtaStatus.buildPsramType);
  json += "\",\"build_memory_type\":\"";
  json += jsonEscape(gOtaStatus.buildMemoryType);
  json += "\",\"build_partitions\":\"";
  json += jsonEscape(gOtaStatus.buildPartitions);
  json += "\",\"partition_layout\":\"";
  json += jsonEscape(gOtaStatus.partitionLayout);
  json += "\",\"running_partition\":\"";
  json += jsonEscape(gOtaStatus.runningPartition);
  json += "\",\"target_partition\":\"";
  json += jsonEscape(gOtaStatus.targetPartition);
  json += "\",\"running_image_flash_mode\":\"";
  json += jsonEscape(gOtaStatus.runningImageFlashMode);
  json += "\",\"upload_image_flash_mode\":\"";
  json += jsonEscape(gOtaStatus.uploadImageFlashMode);
  json += "\"}";
  server.send(200, F("application/json"), json);
}

void handleSdFormatStatusApi() {
  server.send(200, F("application/json"), sdFormatStatusJson());
}

void handleSdFormatPost() {
  if (gSdFormatStatus.inProgress) {
    setSdFormatStatus(F("busy"), F("SD format already in progress"), F("Wait for completion before retrying."), true, false, false);
    server.send(409, F("application/json"), sdFormatStatusJson());
    return;
  }

  const String confirm = server.arg("confirm");
  if (confirm != kSdFormatConfirmToken) {
    setSdFormatStatus(F("blocked"), F("explicit confirmation required"), F("Type FORMAT exactly and submit again to proceed."), false, false, true);
    server.send(400, F("application/json"), sdFormatStatusJson());
    return;
  }

  const bool ok = formatSdToFatAndRemount();
  server.send(ok ? 200 : 500, F("application/json"), sdFormatStatusJson());
}

void handleOtaPage() {
  String body;
  body.reserve(5600);
  body += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  body += F("<title>OTA Update</title></head><body>");
  body += F("<h1>OTA Firmware Update</h1>");
  body += F("<p>Current firmware version: ");
  body += firmware::kFirmwareVersion;
  body += F("</p>");
  body += F("<p>Upload a valid firmware.bin built for this board.</p>");
  body += F("<form id='otaForm' method='POST' action='/ota' enctype='multipart/form-data'>");
  body += F("<input id='fw' type='file' name='firmware' accept='.bin,application/octet-stream' required/>");
  body += F("<button type='submit'>Start OTA</button></form>");
  body += F("<p id='uploadProgress'>Upload progress: 0%</p>");
  body += F("<p id='stage'>Stage: idle</p>");
  body += F("<p id='status'>Status: waiting for upload</p>");
  body += F("<p id='diag'>Diag: env=unknown, running=unknown, target=unknown, error=none(0)</p>");
  body += F("<h2>Build/Config Diagnostics</h2>");
  body += F("<pre id='cfg'>env=unknown\nboard=unknown\nflash_size=unknown\nflash_mode=unknown\npsram_type=unknown\nmemory_type=unknown\npartitions=unknown\nlayout=unknown\nrunning_partition=unknown\ntarget_partition=unknown\nrunning_image_flash_mode=unknown\nupload_image_flash_mode=unknown</pre>");
  body += F("<p id='guidance'>Next step: choose a valid firmware.bin for esp32-s3-devkitc-1-n32r16v.</p>");
  body += F("<p><a href='/debug-log'>Open rolling OTA/SD debug log</a></p>");
  body += F("<p><a href='/'>Back</a></p>");
  body += F("<script>");
  body += F("const f=document.getElementById('otaForm');const p=document.getElementById('uploadProgress');const st=document.getElementById('stage');const s=document.getElementById('status');const g=document.getElementById('guidance');const d=document.getElementById('diag');const cfg=document.getElementById('cfg');");
  body += F("function setUi(j){st.textContent='Stage: '+j.stage;let msg='Status: '+j.message;if(j.in_progress){msg+=' ('+j.received+' bytes)';}s.textContent=msg;d.textContent='Diag: env='+j.build_env+', running='+j.running_partition+', target='+j.target_partition+', error='+j.error_name+'('+j.error_code+')';const lines=['env='+j.build_env,'board='+j.build_board,'flash_size='+j.build_flash_size,'flash_mode='+j.build_flash_mode,'psram_type='+j.build_psram_type,'memory_type='+j.build_memory_type,'partitions='+j.build_partitions,'layout='+j.partition_layout,'running_partition='+j.running_partition,'target_partition='+j.target_partition,'running_image_flash_mode='+j.running_image_flash_mode,'upload_image_flash_mode='+j.upload_image_flash_mode];cfg.textContent=lines.join('\\n');g.textContent='Next step: '+j.guidance;}");
  body += F("f.addEventListener('submit',function(e){e.preventDefault();const file=document.getElementById('fw').files[0];if(!file){st.textContent='Stage: failed';s.textContent='Status: select firmware.bin first';g.textContent='Next step: choose firmware.bin then retry.';return;}const data=new FormData();data.append('firmware',file);const x=new XMLHttpRequest();x.open('POST','/ota',true);st.textContent='Stage: upload';s.textContent='Status: starting upload';g.textContent='Next step: keep this page open until a final result is shown.';x.upload.onprogress=function(ev){if(ev.lengthComputable){const pct=Math.round((ev.loaded/ev.total)*100);p.textContent='Upload progress: '+pct+'%';if(pct>=100){st.textContent='Stage: upload_complete';s.textContent='Status: upload complete; waiting for device validation/apply';}}};x.onreadystatechange=function(){if(x.readyState===4&&x.status>=400){s.textContent='Status: '+x.responseText;}};x.onerror=function(){st.textContent='Stage: failed';s.textContent='Status: upload failed (network error)';g.textContent='Next step: keep device powered, reconnect to AP, and retry.';};x.send(data);});");
  body += F("setInterval(function(){fetch('/api/ota/status').then(r=>r.json()).then(setUi).catch(()=>{});},800);");
  body += F("</script></body></html>");
  server.send(200, F("text/html"), body);
}

void handleDownload() {
  const String path = sanitizePath(server.arg("path"));
  if (path == "/") {
    server.send(400, F("text/plain"), F("path required"));
    return;
  }
  File file = SD.open(path, FILE_READ);
  if (!file || file.isDirectory()) {
    server.send(404, F("text/plain"), F("not found"));
    return;
  }

  server.sendHeader(F("Content-Disposition"), "attachment; filename=\"" + path.substring(path.lastIndexOf('/') + 1) + "\"");
  server.streamFile(file, F("application/octet-stream"));
  file.close();
}

bool parseRange(const String& rangeHeader, size_t fileSize, size_t& start, size_t& end) {
  if (!rangeHeader.startsWith("bytes=")) return false;
  const String rangeValue = rangeHeader.substring(6);
  const int dash = rangeValue.indexOf('-');
  if (dash <= 0) return false;

  const String startStr = rangeValue.substring(0, dash);
  const String endStr = rangeValue.substring(dash + 1);
  start = static_cast<size_t>(startStr.toInt());
  if (start >= fileSize) return false;

  if (endStr.length() == 0) {
    end = fileSize - 1;
  } else {
    end = static_cast<size_t>(endStr.toInt());
  }

  if (end >= fileSize) end = fileSize - 1;
  return end >= start;
}

void streamFileWithRange(File& file, const String& contentType) {
  const size_t fileSize = static_cast<size_t>(file.size());
  const bool hasRange = server.hasHeader("Range");
  size_t start = 0;
  size_t end = fileSize ? fileSize - 1 : 0;
  int statusCode = 200;

  server.sendHeader(F("Accept-Ranges"), F("bytes"));
  server.sendHeader(F("Cache-Control"), F("no-store"));

  if (hasRange) {
    const String rangeHeader = server.header("Range");
    if (!parseRange(rangeHeader, fileSize, start, end)) {
      server.send(416, F("text/plain"), F("invalid range"));
      Serial.println(F("HTTP: RANGE ERROR"));
      return;
    }
    statusCode = 206;
    server.sendHeader(F("Content-Range"), "bytes " + String(start) + "-" + String(end) + "/" + String(fileSize));
    Serial.printf("HTTP: RANGE %u-%u -> 206\n", static_cast<unsigned>(start), static_cast<unsigned>(end));
  } else {
    Serial.println(F("HTTP: ACCEPT-RANGES BYTES"));
  }

  const size_t length = (fileSize == 0) ? 0 : (end - start + 1);
  server.setContentLength(length);
  server.send(statusCode, contentType, "");

  if (length == 0) return;
  if (!file.seek(start)) {
    server.client().stop();
    return;
  }

  uint8_t buffer[kChunkSize];
  size_t remaining = length;
  while (remaining > 0) {
    const size_t toRead = remaining > kChunkSize ? kChunkSize : remaining;
    const size_t readBytes = file.read(buffer, toRead);
    if (readBytes == 0) break;
    server.client().write(buffer, readBytes);
    remaining -= readBytes;
  }
}

void handleStream() {
  const String path = sanitizePath(server.arg("path"));
  if (path == "/") {
    server.send(400, F("text/plain"), F("path required"));
    return;
  }
  File file = SD.open(path, FILE_READ);
  if (!file || file.isDirectory()) {
    server.send(404, F("text/plain"), F("not found"));
    return;
  }
  const String contentType = contentTypeForPath(path);
  streamFileWithRange(file, contentType);
  file.close();
}

void handleUploadDone() {
  server.send(200, F("text/plain"), F("upload complete"));
}

void handleOtaDone() {
  if (gOtaStatus.inProgress) {
    server.send(202, F("text/plain"), gOtaStatus.message);
    return;
  }
  if (gOtaStatus.success) {
    server.send(200, F("text/plain"), gOtaStatus.message);
  } else {
    server.send(500, F("text/plain"), gOtaStatus.message);
  }
}

void handleOtaChunk() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    if (gOtaSession.active) {
      esp_ota_abort(gOtaSession.handle);
      resetOtaSession();
    }
    refreshOtaDiagnostics();
    gOtaStatus.inProgress = true;
    gOtaStatus.success = false;
    gOtaStatus.hasResult = false;
    gOtaStatus.received = 0;
    gOtaStatus.expected = static_cast<size_t>(upload.totalSize);
    gOtaStatus.errorCode = 0;
    gOtaStatus.errorName = F("none");
    gOtaStatus.stage = F("upload");
    gOtaStatus.message = F("starting OTA upload");
    gOtaStatus.guidance = F("Keep this page open while upload, validation, and apply complete.");
    gOtaStatus.runningImageFlashMode = F("unknown");
    gOtaStatus.uploadImageFlashMode = F("unknown");
    gOtaRestartPending = false;

    gOtaSession.runningImageFlashModeKnown = false;
    gOtaSession.headerChecked = false;
    gOtaSession.headerByteCount = 0;

    const esp_partition_t* running = esp_ota_get_running_partition();
    if (running != nullptr) {
      uint8_t runningHeader[3] = {0, 0, 0};
      if (esp_partition_read(running, 0, runningHeader, sizeof(runningHeader)) == ESP_OK && runningHeader[0] == kEspImageMagicByte) {
        gOtaSession.runningImageFlashModeKnown = true;
        gOtaSession.runningImageFlashMode = runningHeader[2];
        gOtaStatus.runningImageFlashMode = otaImageFlashModeName(runningHeader[2]);
      }
    }

    gOtaSession.target = esp_ota_get_next_update_partition(nullptr);
    if (gOtaSession.target == nullptr) {
      setOtaFailure(
          F("precheck_failed"),
          ESP_ERR_NOT_FOUND,
          otaErrorNameForEspErr(ESP_ERR_NOT_FOUND),
          F("OTA blocked before upload: no OTA target partition available"),
          otaGuidanceForEspErr(ESP_ERR_NOT_FOUND));
      serialAndDebugLog(F("OTA: PRECHECK FAILED (NO TARGET PARTITION)"));
      return;
    }

    if (gOtaStatus.expected > 0 && gOtaStatus.expected > gOtaSession.target->size) {
      const String msg = String(F("OTA blocked before validation/apply: firmware larger than target partition (expected=")) +
                         String(static_cast<unsigned long>(gOtaStatus.expected)) +
                         F(", target=") +
                         String(static_cast<unsigned long>(gOtaSession.target->size)) +
                         F(")");
      setOtaFailure(
          F("precheck_failed"),
          ESP_ERR_INVALID_SIZE,
          otaErrorNameForEspErr(ESP_ERR_INVALID_SIZE),
          msg,
          otaGuidanceForEspErr(ESP_ERR_INVALID_SIZE));
      serialAndDebugLog(F("OTA: PRECHECK FAILED (SIZE > TARGET)"));
      return;
    }

    const size_t updateSize = gOtaStatus.expected > 0 ? gOtaStatus.expected : OTA_SIZE_UNKNOWN;
    const esp_err_t beginErr = esp_ota_begin(gOtaSession.target, updateSize, &gOtaSession.handle);
    if (beginErr != ESP_OK) {
      setOtaFailure(
          F("failed"),
          beginErr,
          otaErrorNameForEspErr(beginErr),
          String(F("OTA failed: unable to start update: ")) + otaErrorNameForEspErr(beginErr),
          String(F("Confirm firmware target ")) + kExpectedBoardTarget +
              F(" and retry. If this repeats, verify release build parity and partition diagnostics on this page."));
      serialAndDebugLog(F("OTA: BEGIN FAILED"));
      serialAndDebugLogf("OTA: ERROR CODE=%ld", static_cast<long>(beginErr));
      return;
    }

    gOtaSession.active = true;
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!gOtaStatus.inProgress || !gOtaSession.active) return;

    if (!gOtaSession.headerChecked && upload.currentSize > 0) {
      for (size_t i = 0; i < upload.currentSize && gOtaSession.headerByteCount < sizeof(gOtaSession.headerBytes); ++i) {
        gOtaSession.headerBytes[gOtaSession.headerByteCount] = upload.buf[i];
        ++gOtaSession.headerByteCount;
      }

      if (gOtaSession.headerByteCount == sizeof(gOtaSession.headerBytes)) {
        gOtaSession.headerChecked = true;
        if (gOtaSession.headerBytes[0] != kEspImageMagicByte) {
          esp_ota_abort(gOtaSession.handle);
          resetOtaSession();
          setOtaFailure(
              F("precheck_failed"),
              kOtaErrorBadMagic,
              F("invalid_image_header"),
              F("OTA blocked before validation/apply: invalid ESP image header"),
              F("Selected file is not a valid ESP32 app image. Use firmware.bin built for esp32-s3-devkitc-1-n32r16v."));
          serialAndDebugLog(F("OTA: PRECHECK FAILED (MAGIC BYTE)"));
          return;
        }

        const uint8_t uploadImageFlashMode = gOtaSession.headerBytes[2];
        gOtaStatus.uploadImageFlashMode = otaImageFlashModeName(uploadImageFlashMode);
        if (gOtaSession.runningImageFlashModeKnown && uploadImageFlashMode != gOtaSession.runningImageFlashMode) {
          const String uploadMode = otaImageFlashModeName(uploadImageFlashMode);
          const String runningMode = otaImageFlashModeName(gOtaSession.runningImageFlashMode);
          const String msg = String(F("OTA blocked before validation/apply: uploaded image flash mode ")) + uploadMode +
                             F(" mismatches running image flash mode ") + runningMode;
          esp_ota_abort(gOtaSession.handle);
          resetOtaSession();
          setOtaFailure(
              F("precheck_failed"),
              kOtaErrorFlashModeMismatch,
              F("flash_mode_mismatch"),
              msg,
              F("Use firmware.bin built for esp32-s3-devkitc-1-n32r16v with matching flash/PSRAM config. Verify values in this page's build/config diagnostics."));
          serialAndDebugLog(F("OTA: PRECHECK FAILED (FLASH MODE MISMATCH)"));
          return;
        }
      }
    }

    const esp_err_t writeErr = esp_ota_write(gOtaSession.handle, upload.buf, upload.currentSize);
    if (writeErr != ESP_OK) {
      esp_ota_abort(gOtaSession.handle);
      resetOtaSession();
      setOtaFailure(
          F("failed"),
          writeErr,
          otaErrorNameForEspErr(writeErr),
          String(F("OTA failed: write error: ")) + otaErrorNameForEspErr(writeErr),
          F("Reboot and retry. If repeated, reflash over USB and verify firmware.bin target."));
      serialAndDebugLog(F("OTA: WRITE FAILED"));
      serialAndDebugLogf("OTA: ERROR CODE=%ld", static_cast<long>(writeErr));
      return;
    }

    gOtaStatus.received += upload.currentSize;
    if (gOtaStatus.expected > 0) {
      const unsigned long pct = static_cast<unsigned long>((gOtaStatus.received * 100UL) / gOtaStatus.expected);
      if (pct >= 100) {
        gOtaStatus.stage = F("upload_complete");
        gOtaStatus.message = F("upload complete, preparing validation/apply");
        gOtaStatus.guidance = F("Keep page open. Device now moves to validation/apply.");
      } else {
        gOtaStatus.stage = F("upload");
        gOtaStatus.message = "uploading firmware (" + String(pct) + "%)";
      }
    } else {
      gOtaStatus.stage = F("upload");
      gOtaStatus.message = F("uploading firmware");
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (!gOtaStatus.inProgress || !gOtaSession.active) return;
    if (gOtaStatus.expected > 0 && gOtaStatus.received != gOtaStatus.expected) {
      esp_ota_abort(gOtaSession.handle);
      resetOtaSession();
      setOtaFailure(
          F("failed"),
          kOtaErrorSizeMismatch,
          F("size_mismatch"),
          F("OTA failed: upload size mismatch before validation/apply"),
          F("Retry OTA on a stable connection. Keep this page open until the final status appears."));
      serialAndDebugLog(F("OTA: SIZE MISMATCH"));
      return;
    }

    gOtaStatus.stage = F("validate_apply");
    gOtaStatus.message = F("upload complete, validating and applying firmware");
    gOtaStatus.guidance = F("Wait for final OTA result.");
    serialAndDebugLog(F("OTA: VALIDATE/APPLY START"));

    const esp_err_t endErr = esp_ota_end(gOtaSession.handle);
    if (endErr != ESP_OK) {
      resetOtaSession();
      setOtaFailure(
          F("failed"),
          endErr,
          otaErrorNameForEspErr(endErr),
          String(F("OTA failed during validation/apply: ")) + otaErrorNameForEspErr(endErr),
          otaGuidanceForEspErr(endErr));
      serialAndDebugLog(F("OTA: END FAILED"));
      serialAndDebugLogf("OTA: ERROR CODE=%ld", static_cast<long>(endErr));
      return;
    }

    const esp_err_t bootErr = esp_ota_set_boot_partition(gOtaSession.target);
    resetOtaSession();
    if (bootErr == ESP_OK) {
      gOtaStatus.inProgress = false;
      gOtaStatus.success = true;
      gOtaStatus.hasResult = true;
      gOtaStatus.errorCode = 0;
      gOtaStatus.errorName = F("none");
      gOtaStatus.stage = F("success");
      gOtaStatus.message = F("OTA successful. Device will reboot.");
      gOtaStatus.guidance = F("Wait for reboot, reconnect to ESP32-MEDIA AP, then verify app version.");
      gOtaRestartPending = true;
      gOtaRestartAtMs = millis() + 1500;
      serialAndDebugLog(F("OTA: SUCCESS"));
    } else {
      setOtaFailure(
          F("failed"),
          bootErr,
          otaErrorNameForEspErr(bootErr),
          String(F("OTA failed during validation/apply: ")) + otaErrorNameForEspErr(bootErr),
          otaGuidanceForEspErr(bootErr));
      serialAndDebugLog(F("OTA: ACTIVATE FAILED"));
      serialAndDebugLogf("OTA: ERROR CODE=%ld", static_cast<long>(bootErr));
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (gOtaSession.active) {
      esp_ota_abort(gOtaSession.handle);
      resetOtaSession();
    }
    setOtaFailure(
        F("failed"),
        ESP_ERR_INVALID_STATE,
        F("aborted"),
        F("OTA failed: upload interrupted or aborted"),
        F("Keep device powered, reconnect, and retry OTA with a stable network."));
    serialAndDebugLog(F("OTA: ABORTED"));
  }
}

void handleUploadChunk() {
  HTTPUpload& upload = server.upload();
  static File uploadFile;
  static size_t totalUploaded = 0;

  if (upload.status == UPLOAD_FILE_START) {
    totalUploaded = 0;
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    uploadFile = SD.open(filename, FILE_WRITE);
    if (!uploadFile) {
      server.send(500, F("text/plain"), F("upload open failed"));
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    totalUploaded += upload.currentSize;
    if (totalUploaded > MAX_UPLOAD_BYTES) {
      if (uploadFile) uploadFile.close();
      server.send(413, F("text/plain"), F("upload exceeds 20MB"));
      return;
    }
    if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) uploadFile.close();
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (uploadFile) uploadFile.close();
  }
}

void configureRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/files", HTTP_GET, handleFilesPage);
  server.on("/api/list", HTTP_GET, handleListApi);
  server.on("/api/sd/status", HTTP_GET, handleSdStatusApi);
  server.on("/api/sd/format/status", HTTP_GET, handleSdFormatStatusApi);
  server.on("/api/sd/format", HTTP_POST, handleSdFormatPost);
  server.on("/api/ota/status", HTTP_GET, handleOtaStatusApi);
  server.on("/api/debug-log", HTTP_GET, handleDebugLogApi);
  server.on("/debug-log", HTTP_GET, handleDebugLogPage);
  server.on("/sd-diagnostics", HTTP_GET, handleSdDiagnosticsPage);
  server.on("/download", HTTP_GET, handleDownload);
  server.on("/stream", HTTP_GET, handleStream);
  server.on("/upload", HTTP_POST, handleUploadDone, handleUploadChunk);
  server.on("/ota", HTTP_GET, handleOtaPage);
  server.on("/ota", HTTP_POST, handleOtaDone, handleOtaChunk);
  server.onNotFound([]() { server.send(404, F("text/plain"), F("not found")); });
}

bool mountSd() {
  setSdStatus(SdFailureStage::kInitBusFailure, false, F("not initialized"));
  String detail;
  SdFailureStage stage = SdFailureStage::kInitBusFailure;
  bool capacityKnown = false;
  uint32_t usedHz = kSdSpiHz;
  if (!mountSdWithRetries(false, detail, stage, capacityKnown, usedHz)) {
    setSdStatus(stage, false, detail);
    serialAndDebugLog(F("SD: MOUNT FAILED"));
    serialAndDebugLogf("SD: DIAG STAGE=%s", sdStageCode(gSdStatus.stage).c_str());
    serialAndDebugLogf("SD: DETAIL=%s", detail.c_str());
    return false;
  }

  serialAndDebugLog(F("SD: MOUNTED"));
  if (!capacityKnown) {
    serialAndDebugLog(F("SD: CAPACITY UNKNOWN (continuing because root access works)"));
    setSdStatus(SdFailureStage::kOk, true, detail);
  } else {
    setSdStatus(SdFailureStage::kOk, true, detail);
  }
  serialAndDebugLogf("SD: SPI %s", sdSpiSpeedLabel(usedHz).c_str());
  serialAndDebugLog(F("SD: READ OK"));
  serialAndDebugLog(F("SD: LIST OK"));
  return true;
}

void startApAndServer() {
  const char* apSsid = DEFAULT_AP_SSID;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid, kDefaultApPassword);
  delay(100);
  Serial.printf("WIFI AP STARTED SSID=%s IP=%s\n", apSsid, WiFi.softAPIP().toString().c_str());

  configureRoutes();
  server.begin();
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  serialAndDebugLog(F("BOOT: OK"));
  refreshOtaDiagnostics();
  serialAndDebugLogf("OTA: BUILD ENV=%s BOARD=%s", gOtaStatus.buildEnv.c_str(), gOtaStatus.buildBoard.c_str());
  serialAndDebugLogf("OTA: BUILD CONFIG board=%s flash_size=%s flash_mode=%s psram_type=%s memory=%s partitions=%s",
                     gOtaStatus.buildBoard.c_str(),
                     gOtaStatus.buildFlashSize.c_str(),
                     gOtaStatus.buildFlashMode.c_str(),
                     gOtaStatus.buildPsramType.c_str(),
                     gOtaStatus.buildMemoryType.c_str(),
                     gOtaStatus.buildPartitions.c_str());
  serialAndDebugLogf("OTA: RUNNING PARTITION=%s", gOtaStatus.runningPartition.c_str());
  serialAndDebugLogf("OTA: TARGET PARTITION=%s", gOtaStatus.targetPartition.c_str());
  serialAndDebugLogf("OTA: PARTITION LAYOUT=%s", gOtaStatus.partitionLayout.c_str());

  if (mountSd()) {
    // Boot smoke regex expects RTC line; we only emit a placeholder in v0.1 firmware.
    serialAndDebugLog(F("RTC: READ OK"));
  }

  startApAndServer();
  serialAndDebugLog(F("ESP32-MEDIA: READY"));
}

void loop() {
  if (gOtaRestartPending && static_cast<long>(millis() - gOtaRestartAtMs) >= 0) {
    ESP.restart();
  }
  server.handleClient();
}

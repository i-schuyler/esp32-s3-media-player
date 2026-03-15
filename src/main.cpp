#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>

namespace {
WebServer server(80);

constexpr uint8_t kSdCsPin = 13;
constexpr uint8_t kSdSckPin = 12;
constexpr uint8_t kSdMisoPin = 11;
constexpr uint8_t kSdMosiPin = 10;
constexpr const char* kDefaultApPassword = "12345678";
constexpr size_t kChunkSize = 2048;

struct OtaStatus {
  bool inProgress = false;
  bool success = false;
  bool hasResult = false;
  size_t received = 0;
  size_t expected = 0;
  String message = F("idle");
};

OtaStatus gOtaStatus;
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
      return F("SD init/pin bus failed");
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
      return F("Check wiring and pins: CS=IO13, SCK=IO12, MISO=IO11, MOSI=IO10. Confirm 3.3V power and GND.");
    case SdFailureStage::kCardCommFailure:
      return F("Re-seat or replace the card, confirm FAT32 formatting, and verify the card works on another device.");
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
  body += F("<p><a href='/files'>Open file browser</a></p>");
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
  body.reserve(4096);
  body += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  body += F("<title>ESP32 Files</title></head><body><h1>SD Browser</h1>");
  body += F("<p><a href='/sd-diagnostics'>Open SD diagnostics</a></p>");
  body += F("<p><a href='/ota'>Open OTA firmware updater</a></p>");
  body += F("<form method='POST' action='/upload' enctype='multipart/form-data'>");
  body += F("<input type='file' name='file'/>");
  body += F("<button type='submit'>Upload</button></form>");
  appendDirectoryListingHtml(body, SD, "/");
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
  body += F("<p>Serial logs remain available for deeper debugging.</p>");
  body += F("<p><a href='/api/sd/status'>View raw JSON status</a></p>");
  body += F("<p><a href='/files'>Back to file browser</a></p></body></html>");
  server.send(200, F("text/html"), body);
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
  json += ",\"message\":\"";
  json += jsonEscape(gOtaStatus.message);
  json += "\"}";
  server.send(200, F("application/json"), json);
}

void handleOtaPage() {
  String body;
  body.reserve(2600);
  body += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  body += F("<title>OTA Update</title></head><body>");
  body += F("<h1>OTA Firmware Update</h1>");
  body += F("<p>Upload a valid firmware.bin built for this board.</p>");
  body += F("<form id='otaForm' method='POST' action='/ota' enctype='multipart/form-data'>");
  body += F("<input id='fw' type='file' name='firmware' accept='.bin,application/octet-stream' required/>");
  body += F("<button type='submit'>Start OTA</button></form>");
  body += F("<p id='uploadProgress'>Upload progress: 0%</p>");
  body += F("<p id='status'>Status: idle</p>");
  body += F("<p><a href='/'>Back</a></p>");
  body += F("<script>");
  body += F("const f=document.getElementById('otaForm');const p=document.getElementById('uploadProgress');const s=document.getElementById('status');");
  body += F("f.addEventListener('submit',function(e){e.preventDefault();const file=document.getElementById('fw').files[0];if(!file){s.textContent='Status: select firmware.bin first';return;}const data=new FormData();data.append('firmware',file);const x=new XMLHttpRequest();x.open('POST','/ota',true);x.upload.onprogress=function(ev){if(ev.lengthComputable){const pct=Math.round((ev.loaded/ev.total)*100);p.textContent='Upload progress: '+pct+'%';s.textContent='Status: uploading...';}};x.onreadystatechange=function(){if(x.readyState===4){s.textContent='Status: '+x.responseText;}};x.onerror=function(){s.textContent='Status: upload failed (network error)';};x.send(data);});");
  body += F("setInterval(function(){fetch('/api/ota/status').then(r=>r.json()).then(j=>{let msg='Status: '+j.message;if(j.in_progress){msg+=' ('+j.received+' bytes)';}s.textContent=msg;}).catch(()=>{});},1000);");
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
  if (gOtaStatus.success) {
    server.send(200, F("text/plain"), gOtaStatus.message);
  } else {
    server.send(500, F("text/plain"), gOtaStatus.message);
  }
}

void handleOtaChunk() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    gOtaStatus.inProgress = true;
    gOtaStatus.success = false;
    gOtaStatus.hasResult = false;
    gOtaStatus.received = 0;
    gOtaStatus.expected = static_cast<size_t>(upload.totalSize);
    gOtaStatus.message = F("starting OTA update");
    gOtaRestartPending = false;
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      gOtaStatus.inProgress = false;
      gOtaStatus.hasResult = true;
      gOtaStatus.message = F("OTA failed: unable to start update");
      Serial.println(F("OTA: BEGIN FAILED"));
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!gOtaStatus.inProgress) return;
    const size_t written = Update.write(upload.buf, upload.currentSize);
    if (written != upload.currentSize) {
      Update.abort();
      gOtaStatus.inProgress = false;
      gOtaStatus.hasResult = true;
      gOtaStatus.message = F("OTA failed: write error");
      Serial.println(F("OTA: WRITE FAILED"));
      return;
    }
    gOtaStatus.received += upload.currentSize;
    if (gOtaStatus.expected > 0) {
      const unsigned long pct = static_cast<unsigned long>((gOtaStatus.received * 100UL) / gOtaStatus.expected);
      gOtaStatus.message = "uploading firmware (" + String(pct) + "%)";
    } else {
      gOtaStatus.message = F("uploading firmware");
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (!gOtaStatus.inProgress) return;
    if (Update.end(true)) {
      gOtaStatus.inProgress = false;
      gOtaStatus.success = true;
      gOtaStatus.hasResult = true;
      gOtaStatus.message = F("OTA successful. Device will reboot.");
      gOtaRestartPending = true;
      gOtaRestartAtMs = millis() + 1500;
      Serial.println(F("OTA: SUCCESS"));
    } else {
      gOtaStatus.inProgress = false;
      gOtaStatus.hasResult = true;
      gOtaStatus.message = String(F("OTA failed: ")) + Update.errorString();
      Serial.println(F("OTA: END FAILED"));
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    gOtaStatus.inProgress = false;
    gOtaStatus.success = false;
    gOtaStatus.hasResult = true;
    gOtaStatus.message = F("OTA failed: upload interrupted or aborted");
    Serial.println(F("OTA: ABORTED"));
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
  server.on("/api/ota/status", HTTP_GET, handleOtaStatusApi);
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
  SPI.begin(kSdSckPin, kSdMisoPin, kSdMosiPin, kSdCsPin);
  if (!SD.begin(kSdCsPin)) {
    const uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
      setSdStatus(SdFailureStage::kCardCommFailure, false, F("card not detected during SD.begin"));
    } else {
      setSdStatus(SdFailureStage::kInitBusFailure, false, F("SD.begin failed"));
    }
    Serial.println(F("SD: MOUNT FAILED"));
    Serial.printf("SD: DIAG STAGE=%s\n", sdStageCode(gSdStatus.stage).c_str());
    return false;
  }

  Serial.println(F("SD: MOUNTED"));
  if (SD.cardType() == CARD_NONE) {
    setSdStatus(SdFailureStage::kCardCommFailure, false, F("mounted bus but card type is none"));
    Serial.println(F("SD: TIMEOUT"));
    Serial.printf("SD: DIAG STAGE=%s\n", sdStageCode(gSdStatus.stage).c_str());
    return false;
  }
  if (SD.totalBytes() == 0) {
    setSdStatus(SdFailureStage::kMountFsFailure, false, F("filesystem size is zero"));
    Serial.println(F("SD: CRC ERROR"));
    Serial.printf("SD: DIAG STAGE=%s\n", sdStageCode(gSdStatus.stage).c_str());
    return false;
  }
  File root = SD.open("/");
  if (!root || !root.isDirectory()) {
    setSdStatus(SdFailureStage::kReadRootFailure, false, F("root open failed after mount"));
    Serial.printf("SD: DIAG STAGE=%s\n", sdStageCode(gSdStatus.stage).c_str());
    return false;
  }
  setSdStatus(SdFailureStage::kOk, true, F("mounted and root listed"));
  Serial.println(F("SD: READ OK"));
  Serial.println(F("SD: LIST OK"));
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
  Serial.println(F("BOOT: OK"));

  if (mountSd()) {
    // Boot smoke regex expects RTC line; we only emit a placeholder in v0.1 firmware.
    Serial.println(F("RTC: READ OK"));
  }

  startApAndServer();
  Serial.println(F("ESP32-MEDIA: READY"));
}

void loop() {
  if (gOtaRestartPending && static_cast<long>(millis() - gOtaRestartAtMs) >= 0) {
    ESP.restart();
  }
  server.handleClient();
}

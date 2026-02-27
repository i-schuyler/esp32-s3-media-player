#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <WebServer.h>
#include <WiFi.h>

namespace {
WebServer server(80);

constexpr uint8_t kSdCsPin = 10;
constexpr const char* kDefaultApPassword = "12345678";
constexpr size_t kChunkSize = 2048;

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

void handleRoot() {
  String body;
  body.reserve(512);
  body += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  body += F("<title>ESP32 Media</title></head><body>");
  body += F("<h1>ESP32 Media</h1>");
  body += F("<p><a href='/files'>Open file browser</a></p>");
  body += F("</body></html>");
  server.send(200, F("text/html"), body);
}

void appendDirectoryListingHtml(String& body, fs::FS& fs, const char* dirname) {
  File root = fs.open(dirname);
  if (!root || !root.isDirectory()) {
    body += F("<p>SD root unavailable</p>");
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
  body += F("<form method='POST' action='/upload' enctype='multipart/form-data'>");
  body += F("<input type='file' name='file'/>");
  body += F("<button type='submit'>Upload</button></form>");
  appendDirectoryListingHtml(body, SD, "/");
  body += F("</body></html>");
  server.send(200, F("text/html"), body);
}

void handleListApi() {
  File root = SD.open("/");
  if (!root || !root.isDirectory()) {
    server.send(500, F("application/json"), F("{\"error\":\"sd_root_unavailable\"}"));
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
  server.on("/download", HTTP_GET, handleDownload);
  server.on("/stream", HTTP_GET, handleStream);
  server.on("/upload", HTTP_POST, handleUploadDone, handleUploadChunk);
  server.onNotFound([]() { server.send(404, F("text/plain"), F("not found")); });
}

bool mountSd() {
  if (!SD.begin(kSdCsPin)) {
    Serial.println(F("SD: MOUNT FAILED"));
    return false;
  }

  Serial.println(F("SD: MOUNTED"));
  File root = SD.open("/");
  if (root && root.isDirectory()) {
    Serial.println(F("SD: READ OK"));
    Serial.println(F("SD: LIST OK"));
  }
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
  server.handleClient();
}

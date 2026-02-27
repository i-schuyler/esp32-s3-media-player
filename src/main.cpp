#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <WebServer.h>
#include <WiFi.h>

#ifndef APP_AP_SSID
#define APP_AP_SSID "ESP32-MEDIA"
#endif

#ifndef APP_AP_PASSWORD
#define APP_AP_PASSWORD "12345678"
#endif

#ifndef APP_SD_CS_PIN
#define APP_SD_CS_PIN 10
#endif

#ifndef APP_SD_SPI_FREQ
#define APP_SD_SPI_FREQ 12000000
#endif

static const size_t kUploadLimitBytes = 20U * 1024U * 1024U;

WebServer server(80);
bool gSdMounted = false;
File gUploadFile;
bool gUploadFailed = false;
String gUploadError;

String htmlEscape(const String &input) {
  String out;
  out.reserve(input.length() + 8);
  for (size_t i = 0; i < input.length(); ++i) {
    char c = input[i];
    switch (c) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      case '"':
        out += "&quot;";
        break;
      default:
        out += c;
        break;
    }
  }
  return out;
}

String urlDecode(const String &input) {
  String out;
  out.reserve(input.length());
  for (size_t i = 0; i < input.length(); ++i) {
    const char c = input[i];
    if (c == '+') {
      out += ' ';
      continue;
    }

    if (c == '%' && i + 2 < input.length()) {
      char hi = input[i + 1];
      char lo = input[i + 2];
      auto hex = [](char x) -> int {
        if (x >= '0' && x <= '9') return x - '0';
        if (x >= 'A' && x <= 'F') return 10 + (x - 'A');
        if (x >= 'a' && x <= 'f') return 10 + (x - 'a');
        return -1;
      };
      const int h = hex(hi);
      const int l = hex(lo);
      if (h >= 0 && l >= 0) {
        out += static_cast<char>((h << 4) | l);
        i += 2;
        continue;
      }
    }

    out += c;
  }
  return out;
}

bool isSafePath(const String &path) {
  return path.startsWith("/") && path.indexOf("..") < 0;
}

String contentTypeForPath(const String &path) {
  if (path.endsWith(".htm") || path.endsWith(".html")) return "text/html";
  if (path.endsWith(".css")) return "text/css";
  if (path.endsWith(".js")) return "application/javascript";
  if (path.endsWith(".json")) return "application/json";
  if (path.endsWith(".txt")) return "text/plain";
  if (path.endsWith(".mp3")) return "audio/mpeg";
  if (path.endsWith(".wav")) return "audio/wav";
  if (path.endsWith(".m4a")) return "audio/mp4";
  if (path.endsWith(".mp4")) return "video/mp4";
  if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
  if (path.endsWith(".png")) return "image/png";
  return "application/octet-stream";
}

void sendStatusPage(int statusCode, const String &message) {
  String page;
  page.reserve(256);
  page += "<!doctype html><html><body><h1>ESP32 Media</h1><p>";
  page += htmlEscape(message);
  page += "</p><p><a href=\"/\">Back</a></p></body></html>";
  server.send(statusCode, "text/html", page);
}

void handleIndex() {
  if (!gSdMounted) {
    sendStatusPage(503, "SD not mounted");
    return;
  }

  File root = SD.open("/");
  if (!root || !root.isDirectory()) {
    sendStatusPage(500, "SD root unavailable");
    return;
  }

  String page;
  page.reserve(4096);
  page += "<!doctype html><html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
  page += "<title>ESP32 Media</title></head><body><h1>ESP32 Media</h1>";
  page += "<p>AP: ";
  page += APP_AP_SSID;
  page += "</p>";
  page += "<form method=\"POST\" action=\"/upload\" enctype=\"multipart/form-data\">";
  page += "<input type=\"file\" name=\"file\" required><button type=\"submit\">Upload</button></form>";
  page += "<p>Upload limit: 20MB</p><h2>SD Root</h2><ul>";

  File entry = root.openNextFile();
  while (entry) {
    const String name = String(entry.name());
    const bool isDir = entry.isDirectory();

    page += "<li>";
    page += htmlEscape(name);
    if (isDir) {
      page += " (dir)";
    } else {
      const String encoded = name;
      page += " [<a href=\"/download?path=";
      page += encoded;
      page += "\">download</a>] [<a href=\"/stream?path=";
      page += encoded;
      page += "\">stream</a>]";
      page += " (";
      page += String(static_cast<unsigned long>(entry.size()));
      page += " bytes)";
    }
    page += "</li>";

    entry = root.openNextFile();
  }

  page += "</ul></body></html>";
  server.send(200, "text/html", page);
}

void handleDownload() {
  if (!gSdMounted) {
    sendStatusPage(503, "SD not mounted");
    return;
  }

  if (!server.hasArg("path")) {
    sendStatusPage(400, "Missing path query parameter");
    return;
  }

  const String path = urlDecode(server.arg("path"));
  if (!isSafePath(path)) {
    sendStatusPage(400, "Invalid path");
    return;
  }

  File file = SD.open(path, FILE_READ);
  if (!file || file.isDirectory()) {
    sendStatusPage(404, "File not found");
    return;
  }

  const int slash = path.lastIndexOf('/');
  const String filename = (slash >= 0) ? path.substring(slash + 1) : path;
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
  server.streamFile(file, contentTypeForPath(path));
  file.close();
}

bool parseRange(const String &rangeHeader, size_t fileSize, size_t &start, size_t &end) {
  if (!rangeHeader.startsWith("bytes=")) {
    return false;
  }

  const String value = rangeHeader.substring(6);
  const int dash = value.indexOf('-');
  if (dash < 0) {
    return false;
  }

  const String left = value.substring(0, dash);
  const String right = value.substring(dash + 1);

  if (left.length() == 0) {
    return false;
  }

  const long parsedStart = left.toInt();
  if (parsedStart < 0 || static_cast<size_t>(parsedStart) >= fileSize) {
    return false;
  }

  size_t parsedEnd = fileSize - 1;
  if (right.length() > 0) {
    const long rightValue = right.toInt();
    if (rightValue < 0) {
      return false;
    }
    parsedEnd = static_cast<size_t>(rightValue);
  }

  if (parsedEnd >= fileSize) {
    parsedEnd = fileSize - 1;
  }

  if (parsedEnd < static_cast<size_t>(parsedStart)) {
    return false;
  }

  start = static_cast<size_t>(parsedStart);
  end = parsedEnd;
  return true;
}

void streamFileRange(File &file, const String &path, size_t start, size_t end, bool partial) {
  const size_t fileSize = file.size();
  const size_t contentLen = (end - start) + 1U;
  const String mime = contentTypeForPath(path);

  WiFiClient client = server.client();
  if (!client) {
    return;
  }

  if (partial) {
    client.print("HTTP/1.1 206 Partial Content\r\n");
  } else {
    client.print("HTTP/1.1 200 OK\r\n");
  }
  client.print("Content-Type: " + mime + "\r\n");
  client.print("Accept-Ranges: bytes\r\n");
  client.print("Cache-Control: no-store\r\n");
  if (partial) {
    client.print("Content-Range: bytes " + String(static_cast<unsigned long>(start)) + "-" +
                 String(static_cast<unsigned long>(end)) + "/" +
                 String(static_cast<unsigned long>(fileSize)) + "\r\n");
  }
  client.print("Content-Length: " + String(static_cast<unsigned long>(contentLen)) + "\r\n");
  client.print("Connection: close\r\n\r\n");

  file.seek(start);
  uint8_t buffer[1024];
  size_t remaining = contentLen;
  while (remaining > 0 && client.connected()) {
    const size_t toRead = remaining > sizeof(buffer) ? sizeof(buffer) : remaining;
    const size_t readLen = file.read(buffer, toRead);
    if (readLen == 0) {
      break;
    }
    client.write(buffer, readLen);
    remaining -= readLen;
  }

  client.flush();
}

void handleStream() {
  if (!gSdMounted) {
    sendStatusPage(503, "SD not mounted");
    return;
  }

  if (!server.hasArg("path")) {
    sendStatusPage(400, "Missing path query parameter");
    return;
  }

  const String path = urlDecode(server.arg("path"));
  if (!isSafePath(path)) {
    sendStatusPage(400, "Invalid path");
    return;
  }

  File file = SD.open(path, FILE_READ);
  if (!file || file.isDirectory()) {
    sendStatusPage(404, "File not found");
    return;
  }

  const size_t fileSize = file.size();
  if (fileSize == 0) {
    sendStatusPage(400, "Cannot stream empty file");
    file.close();
    return;
  }

  size_t start = 0;
  size_t end = fileSize - 1;
  bool partial = false;

  if (server.hasHeader("Range")) {
    const String rangeHeader = server.header("Range");
    partial = parseRange(rangeHeader, fileSize, start, end);
    if (!partial) {
      server.sendHeader("Content-Range", "bytes */" + String(static_cast<unsigned long>(fileSize)));
      server.send(416, "text/plain", "Invalid range");
      Serial.println("HTTP: RANGE ERROR");
      file.close();
      return;
    }

    if (start == 0 && end == 1023) {
      Serial.println("HTTP: RANGE 0-1023 -> 206");
    }
  }

  Serial.println("HTTP: ACCEPT-RANGES BYTES");
  streamFileRange(file, path, start, end, partial);
  file.close();
}

void handleUpload() {
  HTTPUpload &upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    gUploadFailed = false;
    gUploadError = "";

    if (!gSdMounted) {
      gUploadFailed = true;
      gUploadError = "SD not mounted";
      return;
    }

    String filename = upload.filename;
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }

    if (!isSafePath(filename)) {
      gUploadFailed = true;
      gUploadError = "Invalid upload filename";
      return;
    }

    if (SD.exists(filename)) {
      SD.remove(filename);
    }

    gUploadFile = SD.open(filename, FILE_WRITE);
    if (!gUploadFile) {
      gUploadFailed = true;
      gUploadError = "Failed to open file for upload";
      return;
    }
  }

  if (upload.status == UPLOAD_FILE_WRITE) {
    if (gUploadFailed || !gUploadFile) {
      return;
    }

    if ((upload.totalSize + upload.currentSize) > kUploadLimitBytes) {
      gUploadFailed = true;
      gUploadError = "Upload exceeds 20MB limit";
      gUploadFile.close();
      return;
    }

    if (gUploadFile.write(upload.buf, upload.currentSize) != upload.currentSize) {
      gUploadFailed = true;
      gUploadError = "Upload write failed";
      gUploadFile.close();
      return;
    }
  }

  if (upload.status == UPLOAD_FILE_END || upload.status == UPLOAD_FILE_ABORTED) {
    if (gUploadFile) {
      gUploadFile.close();
    }

    if (upload.status == UPLOAD_FILE_ABORTED && gUploadError.length() == 0) {
      gUploadFailed = true;
      gUploadError = "Upload aborted";
    }
  }
}

void setupRoutes() {
  const char *headerKeys[] = {"Range"};
  server.collectHeaders(headerKeys, 1);

  server.on("/", HTTP_GET, handleIndex);
  server.on("/download", HTTP_GET, handleDownload);
  server.on("/stream", HTTP_GET, handleStream);

  server.on(
      "/upload", HTTP_POST,
      []() {
        if (gUploadFailed) {
          sendStatusPage(413, gUploadError);
          return;
        }
        server.sendHeader("Location", "/");
        server.send(303);
      },
      handleUpload);

  server.onNotFound([]() { sendStatusPage(404, "Not found"); });
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("BOOT: OK");

  WiFi.mode(WIFI_AP);
  const bool apOk = WiFi.softAP(APP_AP_SSID, APP_AP_PASSWORD);
  if (apOk) {
    Serial.println("WIFI AP STARTED");
  } else {
    Serial.println("WIFI AP FAILED");
  }

  IPAddress apIp = WiFi.softAPIP();
  Serial.print("AP IP: ");
  Serial.println(apIp);

  SPI.begin();
  gSdMounted = SD.begin(APP_SD_CS_PIN, SPI, APP_SD_SPI_FREQ);
  if (gSdMounted) {
    Serial.println("SD: MOUNTED");
    File root = SD.open("/");
    if (root && root.isDirectory()) {
      Serial.println("SD: LIST OK");
      Serial.println("SD: READ OK");
    }
  } else {
    Serial.println("SD: MOUNT FAILED");
  }

  setupRoutes();
  server.begin();
  Serial.println("ESP32-MEDIA: READY");
}

void loop() { server.handleClient(); }

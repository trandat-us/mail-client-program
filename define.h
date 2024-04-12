#ifndef DEFINE_H
#define DEFINE_H

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <random>
#include <map>
#include <thread>
#include <nlohmann/json.hpp>
using namespace std;
using json = nlohmann::json;

#define BUFFER_SIZE 2048
#define FILE_DATA_PER_LINE 72
#define MAX_FILE_SIZE 3145728
#define MAIL_BOX_PATH "emailBox.json"

int smtp_port;
int pop3_port;
int autoload;

string server_ip;
string client_name;
string client_email;
string client_pass;
string client_ip;
map<string, json> FilterInfo;
json MailBox;

// List of supported content-types
map<string, string> contentTypeMapping = {
  {".txt", "text/plain"}, {".html", "text/html"}, {".htm", "text/html"},
  {".pdf", "application/pdf"}, {".doc", "application/msword"},
  {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
  {".xls", "application/vnd.ms-excel"}, {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
  {".ppt", "application/vnd.ms-powerpoint"}, {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
  {".jpeg", "image/jpeg"} , {".jpg", "image/jpeg"}, {".png", "image/png"},
  {".mp3", "audio/mpeg"}, {".wav", "audio/wav"}, 
  {".mp4", "video/mp4"},
  {".zip", "application/zip"}, {".tar", "application/x-tar"}
};

#endif

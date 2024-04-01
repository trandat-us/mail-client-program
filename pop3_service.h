#include "define.h"
#include "base64.h"
#include "utility.h"

vector<string> splitList(string str){
  vector<string> li;
  if(str.find(',') == string::npos)
    li.push_back(str.substr(1));
  else {
    string addr;
    stringstream temp(str);

    while(getline(temp, addr, ',')){
      addr = addr.substr(1);
      li.push_back(addr);
    }
  }
  return li;
}

void readEmail(int boxIndex, int emailIndex){
  MailBox["boxList"][boxIndex]["emailList"][emailIndex]["Read"] = true;
  string is_save;

  cout << "####################################################\n";
  cout << "Email " << emailIndex + 1 << ":" << endl;
  cout << MailBox["boxList"][boxIndex]["emailList"][emailIndex]["From"].get<string>() << ", " 
       << MailBox["boxList"][boxIndex]["emailList"][emailIndex]["Subject"].get<string>() << endl;
  
  cout << "Date: " << MailBox["boxList"][boxIndex]["emailList"][emailIndex]["Date"].get<string>() << endl;
  cout << MailBox["boxList"][boxIndex]["emailList"][emailIndex]["Content"].get<string>() << endl;

  int numbAttach = MailBox["boxList"][boxIndex]["emailList"][emailIndex]["Attachment"].size();
  if(numbAttach > 0){
    cout << "There's(re) " << numbAttach << " attachment(s) in this email, do you want to save it(them)?[Y/n]: ";
    getline(cin, is_save);
    if(is_save == "Y" or is_save == "y"){
      string locate;
      for(int i = 0; i < numbAttach; i++){
        cout << "Location for " << MailBox["boxList"][boxIndex]["emailList"][emailIndex]["Attachment"][i]["filename"].get<string>() << ": ";
        getline(cin, locate);
        try {
          ofstream savefile((locate + "/" + MailBox["boxList"][boxIndex]["emailList"][emailIndex]["Attachment"][i]["filename"].get<string>()), ios::binary);
          savefile << base64_decode(MailBox["boxList"][boxIndex]["emailList"][emailIndex]["Attachment"][i]["data"].get<string>());
          savefile.close();
        }
        catch(...){
          error("Error to detect the location");
        }
      }
    }
  }

  ofstream jsonOutput(MAIL_BOX_PATH);
  jsonOutput << MailBox.dump(2);
  jsonOutput.close();
}

void showEmailBox(int boxIndex){
  string choice;
  int numbEmail = MailBox["boxList"][boxIndex]["emailList"].size();

  while(1){
    cout << "####################################################\n";
    for(int i = 0; i < numbEmail; i++) {
      cout << i + 1 << ". " << (MailBox["boxList"][boxIndex]["emailList"][i]["Read"].get<bool>() ? "" : "(Unread) ")
           << MailBox["boxList"][boxIndex]["emailList"][i]["From"].get<string>()
           << ", " << MailBox["boxList"][boxIndex]["emailList"][i]["Subject"].get<string>() << endl;
    }

    cout << "\nWhich email you want to view: "; 
    getline(cin, choice);

    if(choice.empty())
      break;

    try {
      int emailIndex = stoi(choice);
      if(emailIndex >= 1 && emailIndex <= numbEmail){
        readEmail(boxIndex, emailIndex - 1);
      }
    } catch(...) {
      break;
    }
  }
}

void viewEmail(){
  string choice;
  int numbBox = MailBox["boxList"].size();

  while(1){
    cout << "####################################################\n";
    for(int i = 0; i < numbBox; i++)
      cout << i + 1 << ". " << MailBox["boxList"][i]["boxName"].get<string>() << endl;

    cout << "\nWhich box you want to view: "; 
    getline(cin, choice);

    if(choice.empty())
      break;

    try {
      int boxIndex = stoi(choice);
      if(boxIndex >= 1 && boxIndex <= numbBox)
        showEmailBox(boxIndex - 1);

    } catch(...) {
      break;
    }
  }
}

string getEmailDataResponse(int sockPOP3, int id){
  string data;
  string retr_cm = "RETR " + to_string(id) + "\r\n";
  char response[BUFFER_SIZE];
  bzero(response, BUFFER_SIZE);

  if(send(sockPOP3, retr_cm.c_str(), retr_cm.length(), 0) < 0)
    error("Error: Failed to send RETR command\n");

  while(recv(sockPOP3, response, BUFFER_SIZE - 1, 0)){
    data += response;
    bzero(response, BUFFER_SIZE);
      
    if(data.size() >= 3 && data.substr(data.length() - 3) == ".\r\n")
      break;
  }
  data = data.substr(data.find("\r\n") + 2);

  return data;
}

json parseEmail(string emailData){
  stringstream ss(emailData);
  string from, date, subject, content;
  vector<string> to, cc, bcc;
  vector<pair<string, string>> attach;
  bool inAttach = false;

  string temp, line, attachName, attachData;

  while(getline(ss, line)){
    line.pop_back();
    if(inAttach){
      if(line.find("--------------") == 0){
        attach.push_back(make_pair(attachName, attachData));
        attachData = "";
        inAttach = false;
      }
      else 
        attachData += line;
    }
    else if(line.find("From: ") == 0)
      from = line.substr(6);
    else if(line.find("To: ") == 0) {
      temp = line.substr(3);
      to = splitList(temp);
    }
    else if(line.find("Cc: ") == 0) {
      temp = line.substr(3);
      cc = splitList(temp);
    }
    else if(line.find("Bcc: ") == 0) {
      temp = line.substr(4);
      bcc = splitList(temp);
    }
    else if(line.find("Date: ") == 0) 
      date = line.substr(6);
    else if(line.find("Subject: ") == 0) 
      subject = line.substr(9);
    else if(line.find("Content-Transfer-Encoding: 7bit") == 0){
      getline(ss, line);
      getline(ss, content);
      content.pop_back();
    }
    else if(line.find("Content-Disposition: attachment") == 0){
      inAttach = true;
      char t[100];
      if(sscanf(line.c_str(), "Content-Disposition: attachment; filename=\"%99[^\"]\"", t) == 1)
        attachName = t;

      getline(ss, line);
      getline(ss, line);
    }
  }

  json newEmail = {
    {"Read", false},
    {"From", from},
    {"To", json::array()},
    {"Cc", json::array()},
    {"Bcc", json::array()},
    {"Date", date},
    {"Subject", subject},
    {"Content", content},
    {"Attachment", json::array()}
  };

  for(string k : to)
    newEmail["To"].push_back(k);

  for(string k : cc)
    newEmail["Cc"].push_back(k);

  for(string k : bcc)
    newEmail["Bcc"].push_back(k);

  for(pair<string, string> k : attach){
    json newAttach = {
      {"filename", k.first},
      {"data", k.second}
    };
    newEmail["Attachment"].push_back(newAttach);
  }

  return newEmail;
}

int getBoxIndex(string boxName){
  for(int i = 0; i < MailBox["boxList"].size(); i++){
    if(MailBox["boxList"][i]["boxName"].get<string>() == boxName)
      return i;
  }
  return -1;
}

void FilterAndLoad(json newEmail){
  for(auto it = FilterInfo.begin(); it != FilterInfo.end(); ++it) {
    string key = it->first;
    json value = it->second;
    for(int k = 0; k < value["check"].size(); k++){
      if(key == "Spam"){
        if(newEmail["Subject"].get<string>().find(value["check"][k].get<string>()) != string::npos 
          || newEmail["Content"].get<string>().find(value["check"][k].get<string>()) != string::npos){
          int boxIndex = getBoxIndex("Spam");
          MailBox["boxList"][boxIndex]["emailList"].push_back(newEmail);
          return;
        }
      }
      else {
        if(newEmail[key].get<string>().find(value["check"][k].get<string>()) != string::npos){
          int boxIndex = getBoxIndex(value["dest"].get<string>());
          MailBox["boxList"][boxIndex]["emailList"].push_back(newEmail);
          return;
        }
      }
    }
  }

  MailBox["boxList"][getBoxIndex("Inbox")]["emailList"].push_back(newEmail);
}

void UpdateMailBox(){
  int sockPOP3;
  struct sockaddr_in POP3_addr;
  char response[BUFFER_SIZE];

  // Initialize socket
  sockPOP3 = socket(AF_INET, SOCK_STREAM, 0);
  if (sockPOP3 < 0)
    error("Error initializing socket");

  // Configure socket object
  bzero((char*)&POP3_addr, sizeof(POP3_addr));
  POP3_addr.sin_family = AF_INET;
  POP3_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
  POP3_addr.sin_port = htons(pop3_port);

  // Connect to POP3 Server
  if(connect(sockPOP3, (struct sockaddr*)&POP3_addr, sizeof(POP3_addr)) < 0)
    error("Connection failed");
  bzero(response, BUFFER_SIZE);
  recv(sockPOP3, response, BUFFER_SIZE - 1, 0);

  // Send USER command
  string user_cm = "USER " + client_email + "\r\n";
  if(send(sockPOP3, user_cm.c_str(), user_cm.length(), 0) < 0)
    error("Error: Failed to send USER command\n");
  bzero(response, BUFFER_SIZE);
  recv(sockPOP3, response, BUFFER_SIZE - 1, 0);

  // Send PASS command
  string pass_cm = "PASS " + client_pass + "\r\n";
  if(send(sockPOP3, pass_cm.c_str(), pass_cm.length(), 0) < 0)
    error("Error: Failed to send PASS command\n");
  bzero(response, BUFFER_SIZE);
  recv(sockPOP3, response, BUFFER_SIZE - 1, 0);

  // Send STAT command
  string stat_cm = "STAT\r\n";
  if(send(sockPOP3, stat_cm.c_str(), stat_cm.length(), 0) < 0)
    error("Error: Failed to send PASS command\n");
  bzero(response, BUFFER_SIZE);
  recv(sockPOP3, response, BUFFER_SIZE - 1, 0);

  // Get total email
  int totalEmail;
  sscanf(response, "+OK %d", &totalEmail);

  // check and update
  int emailQuant = MailBox["emailQuant"].get<int>();
  string data;
  json newEmail;

  if(emailQuant < totalEmail){
    for(int i = emailQuant + 1; i <= totalEmail; i++){
      data = getEmailDataResponse(sockPOP3, i);
      newEmail = parseEmail(data);
      FilterAndLoad(newEmail);
    }
    MailBox["emailQuant"] = totalEmail;
  }

  ofstream outputfile(MAIL_BOX_PATH);
  outputfile << MailBox.dump(2);
  outputfile.close();

  close(sockPOP3);
}
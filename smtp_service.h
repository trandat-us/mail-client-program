#include "define.h"
#include "base64.h"
#include "utility.h"

// generate random boundary value for MIME
string generateBoundary() {
  const string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  random_device rd;
  mt19937 gen(rd());
  uniform_int_distribution<> dis(0, chars.size() - 1);

  constexpr int boundaryLength = 24;
  string boundary;
  for (int i = 0; i < boundaryLength; ++i) {
    boundary += chars[dis(gen)];
  }

  return "------------" + boundary;
}

// configure email data with attachments
string createEmailContentWithAttachment(vector<string> to, vector<string> cc, vector<string> bcc,
                            string subject, string content, vector<string> attach){
  string boundary = generateBoundary();
  string emailContent = "Content-Type: multipart/mixed; boundary=\"" + boundary + "\"\r\n"
                        "From: " + client_name + " <" + client_email + ">\r\n"
                        "To:";
  for(int i = 0; i < to.size(); i++){
    if(i != to.size() - 1)
      emailContent += ' ' + to[i] + ',';
    else
      emailContent += ' ' + to[i] + "\r\n";
  }
  
  if(cc.size() != 0){
    emailContent += "Cc:";
    for(int i = 0; i < cc.size(); i++){
      if(i != cc.size() - 1)
        emailContent += ' ' + cc[i] + ',';
      else
        emailContent += ' ' + cc[i] + "\r\n";
    }
  }

  if(bcc.size() != 0){
    emailContent += "Bcc:";
    for(int i = 0; i < bcc.size(); i++){
      if(i != bcc.size() - 1)
        emailContent += ' ' + bcc[i] + ',';
      else
        emailContent += ' ' + bcc[i] + "\r\n";
    }
  }

  const char* date = exec("date");
  emailContent += "Date: " + string(date) + "\r\n"; 
  emailContent += "Subject: " + subject + "\r\n"
                  "\r\n"
                  "--" + boundary + "\r\n"
                  "Content-Type: text/plain; charset=UTF-8; format=flowed\r\n"
                  "Content-Transfer-Encoding: 7bit\r\n"
                  "\r\n" + content + "\r\n"
                  "\r\n";

  int total_size = 0;
  int name_pos, ex_pos;
  string name, extension;
  for(int i = 0; i < attach.size(); i++){
    name_pos = attach[i].find_last_of("/\\");
    if (name_pos != string::npos) 
      name = attach[i].substr(name_pos + 1);
    else
      error(("Cannot detect attachment from path " + attach[i]).c_str());

    ex_pos = name.find_last_of(".");
    if (ex_pos != string::npos) 
      extension = name.substr(ex_pos);
    else
      error(("Cannot detect extension from path " + attach[i]).c_str());
    
    ifstream inAtt(attach[i].c_str(), ifstream::ate | ifstream::binary);
    if(inAtt.tellg() > MAX_FILE_SIZE)
      error(("File " + name + " is larger than 3 MB").c_str());
    
    string fileContent = base64_file(attach[i].c_str());
    int fileContentLength = fileContent.length();   

    emailContent += "--" + boundary + "\r\n"
                    "Content-Type: " + contentTypeMapping[extension] + "; name=\"" + name + "\"\r\n"
                    "Content-Disposition: attachment; filename=\"" + name + "\"\r\n"
                    "Content-Transfer-Encoding: base64\r\n"
                    "\r\n";
    
    int k = 0;
    while(fileContentLength > 0){
      emailContent += fileContent.substr(k * FILE_DATA_PER_LINE, (fileContentLength < FILE_DATA_PER_LINE ? fileContentLength : FILE_DATA_PER_LINE)) + "\r\n";
      k += 1;
      fileContentLength -= FILE_DATA_PER_LINE;
    }
  }
  emailContent += "\r\n"
                  "--" + boundary + "--\r\n"
                  ".\r\n";
  return emailContent;
}

// configure email data without attachment
string createEmailContentNoAttachment(vector<string> to, vector<string> cc, vector<string> bcc,
                            string subject, string content){
  string emailContent = "From: " + client_name + " <" + client_email + ">\r\n"
                        "To:";
  for(int i = 0; i < to.size(); i++){
    if(i != to.size() - 1)
      emailContent += ' ' + to[i] + ',';
    else
      emailContent += ' ' + to[i] + "\r\n";
  }
  
  if(cc.size() != 0){
    emailContent += "Cc:";
    for(int i = 0; i < cc.size(); i++){
      if(i != cc.size() - 1)
        emailContent += ' ' + cc[i] + ',';
      else
        emailContent += ' ' + cc[i] + "\r\n";
    }
  }

  if(bcc.size() != 0){
    emailContent += "Bcc:";
    for(int i = 0; i < bcc.size(); i++){
      if(i != bcc.size() - 1)
        emailContent += ' ' + bcc[i] + ',';
      else
        emailContent += ' ' + bcc[i] + "\r\n";
    }
  }

  const char* date = exec("date");
  emailContent += "Date: " + string(date) + "\r\n"; 
  emailContent += "Subject: " + subject + "\r\n"
                  "Content-Type: text/plain; charset=UTF-8; format=flowed\r\n"
                  "Content-Transfer-Encoding: 7bit\r\n"
                  "\r\n" + content + "\r\n"
                  "\r\n"
                  ".\r\n";
  return emailContent;
}

// insert input data into a vector, which used for adding
// To, Cc and Bcc
vector<string> readList(string title){
  vector<string> rs;
  string temp;
  int i = 1;
  cout << title << ": " << endl; 
  do {
    cout << "- Person [" << i++ << "]: "; getline(cin, temp);
    rs.push_back(temp);
  } while(!temp.empty());
  rs.pop_back();
  return rs;
}

void sendRCPTCommand(int sockSMTP, vector<string> rcpt){
  string cm;
  for(int i = 0; i < rcpt.size(); i++){
    cm = "RCPT TO:<" + rcpt[i] + ">\r\n";
    if(send(sockSMTP, cm.c_str(), cm.length(), 0) < 0)
      error("Error: Failed to send RCPT TO command\n");
  }
}

// perform 'send email' feature
void sendEmail(){
  int sockSMTP;
  struct sockaddr_in SMTP_addr;
  string subject, content, is_attach, emailContent;
  vector<string> to, cc, bcc, attach;

  // Customize email
  cout << "Customize your email info (press enter to skip):" << endl;
  to = readList("To");
  cc = readList("Cc"); 
  bcc = readList("Bcc");
  cout << "Subject: "; getline(cin, subject);
  cout << "Content: "; getline(cin, content);
  cout << "Do you want to attach some files?[Y/n]: "; getline(cin, is_attach);
  if(is_attach == "Y" or is_attach == "y"){
    int numb;
    string path;
    cout << "How many files to be attached: "; cin >> numb;
    cin.ignore();
    for(int i = 0; i < numb; i++){
      cout << "- File path [" << i + 1 << "]: "; getline(cin, path);
      attach.push_back(path);
    }
    emailContent = createEmailContentWithAttachment(to, cc, bcc, subject, content, attach);
  }
  else
    emailContent = createEmailContentNoAttachment(to, cc, bcc, subject, content);

  // Initialize socket
  sockSMTP = socket(AF_INET, SOCK_STREAM, 0);
  if (sockSMTP < 0)
    error("Error initializing socket");

  // Configure socket object
  bzero((char*)&SMTP_addr, sizeof(SMTP_addr));
  SMTP_addr.sin_family = AF_INET;
  SMTP_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
  SMTP_addr.sin_port = htons(smtp_port);

  // Connect to SMTP Server
  if(connect(sockSMTP, (struct sockaddr*)&SMTP_addr, sizeof(SMTP_addr)) < 0)
    error("Connection failed");
  
  // Send EHLO command
  string ehlo_cm = "EHLO [" + client_ip + "]\r\n";
  if(send(sockSMTP, ehlo_cm.c_str(), ehlo_cm.length(), 0) < 0)
    error("Error: Failed to send EHLO command\n");

  // Send MAIL FROM command
  string mail_from_cm = "MAIL FROM:<" + client_email +">\r\n";
  if(send(sockSMTP, mail_from_cm.c_str(), mail_from_cm.length(), 0) < 0)
    error("Error: Failed to send MAIL FROM command\n");

  // Send RCPT TO command
  sendRCPTCommand(sockSMTP, to);
  sendRCPTCommand(sockSMTP, cc);
  sendRCPTCommand(sockSMTP, bcc);
  
  // Send DATA command
  string data_cm = "DATA\r\n";
  if(send(sockSMTP, data_cm.c_str(), data_cm.length(), 0) < 0)
    error("Error: Failed to send MAIL FROM command\n");
  
  // Send DATA's content
  if(send(sockSMTP, emailContent.c_str(), emailContent.length(), 0) < 0)
    error("Error: Failed to send DATA'content command\n");

  // Send QUIT command
  string quit_cm = "QUIT\r\n";
  if(send(sockSMTP, quit_cm.c_str(), quit_cm.length(), 0) < 0)
    error("Error: Failed to send QUIT command\n");

  // Waiting for QUIT response
  char response[BUFFER_SIZE];
  while(1){
    bzero(response, BUFFER_SIZE);
    recv(sockSMTP, response, BUFFER_SIZE - 1, 0);
    if(strncmp(response, "221", 3) == 0)
      break;
  }

  cout << "Sent email successfully" << endl;
  close(sockSMTP);
}

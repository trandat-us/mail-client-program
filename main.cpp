#include "smtp_service.h"
#include "pop3_service.h"

string showMainMenu(){
  string choice;
  cout << "####################################################\n";  
  cout << "1. Send email\n";
  cout << "2. View email\n";
  cout << "3. Exit\n\n";
  cout << "Which action do you take: ";
  getline(cin, choice);
  return choice;
}

void autoUpdateMailBox(){
  while(1){
    UpdateMailBox();
    sleep(autoload);
  }
}

void config(){
  const char* ipCommand = exec("hostname -I");
  char ip[100];
  sscanf(ipCommand, "%99s", ip);
  client_ip = ip;

  ifstream configFile("config.json");
  if(!configFile)
    error("Can't open config file");

  json FilterJson;
  configFile >> FilterJson;
  configFile.close();

  client_name = FilterJson["General"]["Username"].get<string>();
  client_email = FilterJson["General"]["Useremail"].get<string>();
  client_pass = FilterJson["General"]["Password"].get<string>();
  server_ip = FilterJson["General"]["MailServer"].get<string>();
  smtp_port = FilterJson["General"]["SMTP"].get<int>();
  pop3_port = FilterJson["General"]["POP3"].get<int>();
  autoload = FilterJson["General"]["Autoload"].get<int>();

  for(auto it = FilterJson["Filter"].begin(); it != FilterJson["Filter"].end(); ++it)
    FilterInfo[it.key()] = it.value();

  ifstream inputfile(MAIL_BOX_PATH);
  inputfile >> MailBox;
  
  thread updateThread(autoUpdateMailBox);
  updateThread.detach();
}

int main(){
  config();
  string choice;

  while(1){
    choice = showMainMenu();
    try {
      int choiceIndex = stoi(choice);
      if(choiceIndex == 1){
        sendEmail();
        UpdateMailBox();
      }
      else if(choiceIndex == 2)
        viewEmail();
      else {
        system("clear");
        exit(0);
      }
    }
    catch (...){
      system("clear");
      exit(0);
    }
  }
  return 0;
}
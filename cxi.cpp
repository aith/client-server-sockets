// $Id: cxi.cpp,v 1.1 2020-11-22 16:51:43-08 - - $
// All we gotta implement is get, put, and rm? right?
// put: put a file from user onto server
// get: get file from server to user

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

#include <libgen.h>
#include <sys/types.h>
#include <unistd.h>

#include "protocol.h"
#include "logstream.h"
#include "sockets.h"

#include <fstream>

logstream outlog (cout);
struct cxi_exit: public exception {};


unordered_map<string,cxi_command> command_map {
   {"exit", cxi_command::EXIT},
   {"help", cxi_command::HELP},
   {"ls"  , cxi_command::LS  },
   {"put" , cxi_command::PUT  },
   {"get" , cxi_command::GET  },
   {"rm"  , cxi_command::RM  },
};


static const char help[] = R"||(
exit         - Exit the program.  Equivalent to EOF.
get filename - Copy remote file to local host.
help         - Print help summary.
ls           - List names of files on remote server.
put filename - Copy local file to remote host.
rm filename  - Remove file from remote server.
)||";


// From shell sim project
vector<string> split (const string& line, const string& delimiters) {
   vector<string> words;
   size_t end = 0;

   // Loop over the string, splitting out words, and for each word
   // thus found, append it to the output vector.
   for (;;) {
      size_t start = line.find_first_not_of (delimiters, end);
      if (start == string::npos) break;
      end = line.find_first_of (delimiters, start);
      words.push_back (line.substr (start, end - start));
   }
   return words;
}

void cxi_help() {
   cout << help;
}

void cxi_ls (client_socket& server) {
   cxi_header header;
   header.command = cxi_command::LS;
   outlog << "sending header " << header << endl;
   send_packet (server, &header, sizeof header);
   recv_packet (server, &header, sizeof header);
   outlog << "received header " << header << endl;
   if (header.command != cxi_command::LSOUT) {
      outlog << "sent LS, server did not return LSOUT" << endl;
      outlog << "server returned " << header << endl;
   }else {
      size_t host_nbytes = ntohl (header.nbytes);
      auto buffer = make_unique<char[]> (host_nbytes + 1);
      recv_packet (server, buffer.get(), host_nbytes);
      outlog << "received " << host_nbytes << " bytes" << endl;
      buffer[host_nbytes] = '\0';
      cout << buffer.get();
   }
}

// every time we do a send_packet, the other has to recv packet.
void cxi_put (client_socket& server, string& filename) {
   cxi_header header;
   header.command = cxi_command::PUT;
   strcpy(header.filename, filename.c_str());
   ifstream is (header.filename, std::ifstream::binary);
   if (is) {
      is.seekg (0, is.end);
      size_t client_length = is.tellg();
      auto host_nbytes = htonl(client_length); //conv to network endian
      header.nbytes = host_nbytes;
      is.seekg (0, is.beg);

      outlog << "sending header " << header << endl;
      send_packet(server, &header, sizeof header); 
      recv_packet(server, &header, sizeof header); 
      outlog << "received header " << header << endl;
      if (header.command != cxi_command::NAK) {  // Good
         // size_t host_nbytes = htonl (header.nbytes);
         // size_t host_nbytes = htons (header.nbytes);
         // cout << "host_nbytes is " << host_nbytes << endl;
         auto buffer = make_unique<char[]> (client_length + 1);
         // char buffer[client_length];
         is.read (buffer.get(), client_length);
         // is.read (buffer.get(), host_nbytes);
         buffer[client_length] = '\0';  // End of Line at end of buffer
         // buffer[host_nbytes] = '\0';  // End of Line at end of buffer

         outlog << "sending buffer" << endl;
         send_packet(server, buffer.get(), client_length);
         // send_packet(server, buffer.get(), host_nbytes);
         is.close();
      }
      else {  // Bad
         outlog << "error: server could not write file" << endl;
         return;
      }
   }
   else {  
      outlog << "error: server could not write file" << endl;
      return;
   }
}


void cxi_get (client_socket& server, string& filename) {
   cxi_header header;
   header.command = cxi_command::GET;
   // header.nbytes = htonl(0);  // payload at 1 must be nbytes = 0
   strcpy(header.filename, filename.c_str());
   ofstream os (header.filename, std::ofstream::binary);
   outlog << "sending header " << header << endl;
   // 0 goes to server loop
   send_packet(server, &header, sizeof header); 
   // 1 gets from reply_get's if (is) statement
   recv_packet(server, &header, sizeof header); 
   outlog << "received header " << header << endl;
   if(os) {
      if (header.command == cxi_command::FILEOUT) { 
         size_t host_nbytes = ntohl(header.nbytes);
         cout << "bytes to get is " << host_nbytes << endl;
         auto buffer = make_unique<char[]>(host_nbytes + 1);
         // 2
         recv_packet(server, buffer.get(), host_nbytes);
         buffer[host_nbytes] = '\0';
         os.write(buffer.get(), host_nbytes);
         os.close();
      }
      else {
         outlog << "error: could not open ofstream" << endl;
      }
   }
   else {
      outlog << "error: could not open ostream" << endl;
   }
}

void cxi_rm (client_socket& server, string& filename) {
   cxi_header header;
   header.command = cxi_command::RM;
   strcpy(header.filename, filename.c_str());
   // 0
   send_packet(server, &header, sizeof header); 
   recv_packet(server, &header, sizeof header); 
   if (header.command == cxi_command::ACK) { 
      outlog << "File successfully deleted" << endl;
   }
   else {
      outlog << "Could not locate file" << endl;
   }
}

// void cxi_get (client_socket& server, string& filename) {
//    cxi_header header;
//    header.command = cxi_command::GET;
//    strcpy(header.filename, filename.c_str());
//    ofstream os (header.filename, std::ofstream::binary);
//    if (os) {
//       // os.seekg (0, os.end);
//       // int client_length = os.tellg();
//       // auto host_nbytes = htonl(client_length);
//       // header.nbytes = host_nbytes;

//       outlog << "sending header " << header << endl;
//       // 0 goes to server loop
//       send_packet(server, &header, sizeof header); 
//       // 1 gets from reply_get's if (is) statement
//       recv_packet(server, &header, sizeof header); 
//       outlog << "received header " << header << endl;

//       if (header.command == cxi_command::FILEOUT) { 
//          size_t client_size = ntohl(header.nbytes);
//          auto buffer = make_unique<char[]> (client_size + 1);
//          recv_packet (server, buffer.get(), client_size);
//          buffer[client_size] = '\0';  // End of Line at end of buffer

//          // 2 get file data
//          outlog << "received buffer" << endl;
//          os.write(buffer.get(), client_size);
//          os.close();
//       }
//       else { 
//          outlog << "error: server could not open ifstream" << endl;
//       }
//    }
//    else {
//       outlog << "error: could not open ofstream" << endl;
//       header.command = cxi_command::NAK;
//    }
// }

// void cxi_put (client_socket& server, string& filename) {
//    cxi_header header;
//    header.command = cxi_command::PUT;
//    strcpy(header.filename, filename.c_str());
//    send_packet(server, &header, sizeof header); 
//    recv_packet(server, &header, sizeof header); 
//    ifstream is (header.filename, std::ifstream::binary);
//    if (is) {
//       is.seekg (0, is.end);
//       auto client_length = is.tellg();
//       header.nbytes = client_length;  // client_length?
//       // ISSUE: not receiving correct number of bytes
//       is.seekg (0, is.beg);

//       size_t host_nbytes = ntohl (header.nbytes);
//       auto buffer = make_unique<char[]> (host_nbytes + 1);

//       is.read (buffer.get(), client_length);
//       buffer[client_length] = '\0';  // End of Line at end of buffer

//       is.close();
//       send_packet(server, buffer.get(), host_nbytes);
//    }
//    if (header.command != cxi_command::NAK) {  // Good
//    }
//    else {  // Bad
      
//    }
// }


void usage() {
   cerr << "Usage: " << outlog.execname() << " [host] [port]" << endl;
   throw cxi_exit();
}

int main (int argc, char** argv) {
   outlog.execname (basename (argv[0]));
   outlog << "starting" << endl;
   vector<string> args (&argv[1], &argv[argc]);
   if (args.size() > 2) usage();
   string host = get_cxi_server_host (args, 0);
   in_port_t port = get_cxi_server_port (args, 1);
   outlog << to_string (hostinfo()) << endl;
   try {
      outlog << "connecting to " << host << " port " << port << endl;
      client_socket server (host, port);  // ctor attempts to bind
      outlog << "connected to " << to_string (server) << endl;
      for (;;) {
         string line;
         getline (cin, line);
         // * Do split(line) here and you'll get a vector.
         // If you do "put filename", vec[0] will be "put"
         // and vec[1] will be "filename".
         // 
         // theres cases you'll needa account for. such
         // as, if you only do "ls"
         auto words = split(line, " ");
         // use this to look for command
         auto command = words[0];  
         string filename;
         // this might result in error, so put it later
         if (cin.eof()) throw cxi_exit();
         outlog << "command " << line << endl;
         const auto& itor = command_map.find (command);
         cxi_command cmd = itor == command_map.end()
                         ? cxi_command::ERROR : itor->second;
         switch (cmd) {
            case cxi_command::EXIT:
               throw cxi_exit();
               break;
            case cxi_command::HELP:
               cxi_help();
               break;
            case cxi_command::LS:
               cxi_ls (server);
               break;
            case cxi_command::RM:
               if (words.size() < 1) {
                  outlog << "Command requires second input" << endl;
                  break;
               }
               filename = words[1];  
               cxi_rm (server, filename);
               break;
            case cxi_command::PUT:
               if (words.size() < 1) {
                  outlog << "Command requires second input" << endl;
                  break;
               }
               filename = words[1];  
               cxi_put (server, filename);
               break;
            case cxi_command::GET:
               if (words.size() < 1) {
                  outlog << "Command requires second input" << endl;
                  break;
               }
               filename = words[1];  
               cxi_get (server, filename);
               break;
            default:
               outlog << line << ": invalid command" << endl;
               break;
         }
      }
   }catch (socket_error& error) {
      outlog << error.what() << endl;
   }catch (cxi_exit& error) {
      outlog << "caught cxi_exit" << endl;
   }
   outlog << "finishing" << endl;
   return 0;
}

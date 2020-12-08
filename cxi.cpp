// $Id: cxi.cpp,v 1.1 2020-11-22 16:51:43-08 - - $
// Overview: For every send_packet(), there must be
// a recv_packet() on the opposing server or client.
// Data is sent through data packets. The buffer stores
// the byte data that we want.

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
         auto buffer = make_unique<char[]> (client_length + 1);
         is.read (buffer.get(), client_length);
         buffer[client_length] = '\0';  // End of Line at end of buffer

         outlog << "sending buffer" << endl;
         send_packet(server, buffer.get(), client_length);
         outlog << "success!" << endl;
         is.close();
      }
      else {  // Bad output
         cerr << "PUT: Could not write file" << endl;
         outlog << "error: server could not write file" << endl;
         return;
      }
   }
   else {  
      cerr << "PUT: Could not write file" << endl;
      outlog << "error: server could find file" << endl;
      return;
   }
}

void cxi_get (client_socket& server, string& filename) {
   cxi_header header;
   header.command = cxi_command::GET;
   strcpy(header.filename, filename.c_str());
   outlog << "sending header " << header << endl;
   // 0 goes to server loop
   send_packet(server, &header, sizeof header); 
   // 1 gets from reply_get's if (is) statement
   recv_packet(server, &header, sizeof header); 
   outlog << "received header " << header << endl;
   if (header.nbytes != 0) {
      ofstream os (header.filename, std::ofstream::binary);
      if(os) {
         if (header.command == cxi_command::FILEOUT) { 
            size_t host_nbytes = ntohl(header.nbytes);
            auto buffer = make_unique<char[]>(host_nbytes + 1);
            // 2
            outlog << "receiving packet" << endl;
            recv_packet(server, buffer.get(), host_nbytes);
            os.write(buffer.get(), host_nbytes);
            buffer[host_nbytes] = '\0';
            os.close();
            outlog << "success!" << endl;
            }
         }
         else {
            cerr << "PUT: Could not open ofstream" << endl;
            outlog << "error: could not open ofstream" << endl;
         }
   }
   else {
         cerr << "PUT: Could not find file" << endl;
         outlog << "error: could not find file" << endl;
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
      cerr << "RM: Could not locate file" << endl;
      outlog << "Could not locate file" << endl;
   }
}


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
      client_socket server (host, port);  
      outlog << "connected to " << to_string (server) << endl;
      for (;;) {
         string line;
         getline (cin, line);
         auto words = split(line, " ");
         auto command = words[0];  
         string filename;
         if (cin.eof()) throw cxi_exit();
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

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
   ifstream is (header.filename, std::ofstream::binary);
   if (is) {
      is.seekg (0, is.end);
      int client_length = is.tellg();
      auto host_nbytes = htonl(client_length);   // change to network endianness
      header.nbytes = host_nbytes;
      // header.nbytes = client_length;
      is.seekg (0, is.beg);
      // cout << client_length << endl;

      outlog << "sending header " << header << endl;
      send_packet(server, &header, sizeof header); 
      recv_packet(server, &header, sizeof header); 
      outlog << "received header " << header << endl;
      if (header.command != cxi_command::NAK) {  // Good
         // size_t host_nbytes = htonl (header.nbytes);
         // size_t host_nbytes = htons (header.nbytes);
         // cout << "host_nbytes is " << host_nbytes << endl;
         auto buffer = make_unique<char[]> (static_cast<size_t>(client_length) + 1);
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
      // size_t host_nbytes = ntohl (header.nbytes);
      // auto buffer = make_unique<char[]> (host_nbytes + 1);

      // is.read (buffer.get(), client_length);
      // buffer[client_length] = '\0';  // End of Line at end of buffer

      // is.close();
      // send_packet(server, buffer.get(), host_nbytes);
   // }
   // if (header.command != cxi_command::NAK) {  // Good
   // }
   // else {  // Bad
      }
   }
}

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


void cxi_rm (client_socket& server, string& filename) {
   // cxi_header header;
   // strcpy(header.filename, filename.c_str());  // from Tre
   // //didnt finish. I don't think we need more than 1 send packet
   // // size_t host_nbytes = ntohl (header.nbytes);
   // // send_packet(server, buffer.get(), header.nbytes);
   // // recv_packet(server, buffer.get(), header.nbytes);
   // header.command = cxi_command::RM;
   // outlog << "sending header " << header << endl;
   // send_packet (server, &header, sizeof header);
   // recv_packet (server, &header, sizeof header);
   // outlog << "received header " << header << endl;
   // if (header.command != cxi_command::ACK) {  // As per the docs
   //    outlog << "sent RM, server did not return ACK" << endl;
   //    outlog << "server returned " << header << endl;
   // }else {
   //    size_t host_nbytes = ntohl (header.nbytes);
   //    auto buffer = make_unique<char[]> (host_nbytes + 1);
   //    recv_packet (server, buffer.get(), host_nbytes);
   //    outlog << "received " << host_nbytes << " bytes" << endl;
   //    buffer[host_nbytes] = '\0';
   //    cout << buffer.get();
   // }
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
         cout << "command is " << command << endl;
         // this might result in error, so put it later
         auto filename = words[1];  
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
               cxi_rm (server, filename);
               break;
            case cxi_command::PUT:
               cout << "inside put option " << endl;
               cxi_put (server, filename);
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

// $Id: cxid.cpp,v 1.2 2020-11-29 12:38:28-08 - - $

#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include <libgen.h>
#include <sys/types.h>
#include <unistd.h>

#include <memory>
#include "protocol.h"
#include "logstream.h"
#include "sockets.h"

#include <fstream>

logstream outlog (cout);
struct cxi_exit: public exception {};

void reply_ls (accepted_socket& client_sock, cxi_header& header) {
   const char* ls_cmd = "ls -l 2>&1";
   FILE* ls_pipe = popen (ls_cmd, "r");
   if (ls_pipe == NULL) { 
      outlog << ls_cmd << ": " << strerror (errno) << endl;
      header.command = cxi_command::NAK;
      header.nbytes = htonl (errno);
      send_packet (client_sock, &header, sizeof header);
      return;
   }
   string ls_output;
   char buffer[0x1000];
   for (;;) {
      char* rc = fgets (buffer, sizeof buffer, ls_pipe);
      if (rc == nullptr) break;
      ls_output.append (buffer);
   }
   int status = pclose (ls_pipe);
   if (status < 0) outlog << ls_cmd << ": " << strerror (errno) << endl;
              else outlog << ls_cmd << ": exit " << (status >> 8)
                          << " signal " << (status & 0x7F)
                          << " core " << (status >> 7 & 1) << endl;
   header.command = cxi_command::LSOUT;
   header.nbytes = htonl (ls_output.size());
   memset (header.filename, 0, FILENAME_SIZE);
   outlog << "sending header " << header << endl;
   send_packet (client_sock, &header, sizeof header);
   send_packet (client_sock, ls_output.c_str(), ls_output.size());
   outlog << "sent " << ls_output.size() << " bytes" << endl;
}

// void reply_put (accepted_socket& client_sock, cxi_header& header) {
//    // ifstream os (header.filename, ifstream::binary);
//    std::ofstream os (header.filename, std::ofstream::binary);
//    // recv_packet(client_sock, &header, sizeof header); 
//    if(header.nbytes != 0) {
//       if (os) {
//          size_t host_nbytes = ntohl (header.nbytes);
//          auto buffer = make_unique<char[]>(host_nbytes + 1);
//          recv_packet(client_sock, buffer.get(), header.nbytes); 
//          os.write(buffer.get(), host_nbytes+1);  
//          header.command = cxi_command::ACK;
//          os.close();
//       }
//       else {  // File doesn't exist, so send NAK
//          // auto buffer = make_unique<char[]>(0 + 1);
//          // recv_packet(client_sock, buffer.get(), 0); 
//          header.command = cxi_command::NAK;
//       }
//     }
//     else {
//        // Are we supposed to send NAK on empty file?  
//        // recv_packet(client_sock, buffer.get(), header.nbytes); 
//        header.command = cxi_command::NAK;
//        outlog << "nbytes is 0" << endl;
//     }
//     send_packet(client_sock, &header, sizeof header);
// }
// 
void reply_put (accepted_socket& client_sock, cxi_header& header) {
   outlog << "header.filename is " << header.filename << endl;
   std::ofstream os (header.filename, std::ofstream::binary);
   if(header.nbytes != 0) {
      if (os) {
         header.command = cxi_command::ACK;
         send_packet (client_sock, &header, sizeof header);
         // size_t host_nbytes = htonl (header.nbytes);  // server's byte size
         size_t host_nbytes = ntohl (header.nbytes);  // server's byte size
         // auto buffer = make_unique<char[]>(host_nbytes + 1);
         auto buffer = make_unique<char[]>(host_nbytes);
         outlog << "receiving buffer" << endl;
         recv_packet(client_sock, buffer.get(), host_nbytes);  
         outlog << "received buffer" << endl;
         // os.write(buffer.get(), host_nbytes + 1);  
         os.write(buffer.get(), host_nbytes);  
         os.close();
      }
      else {  // File doesn't exist, so send NAK
         outlog << "error: could find file" << endl;
         header.command = cxi_command::NAK;
      }
    }
    else { // could not open ofstream
       // Are we supposed to send NAK on empty file?  
       outlog << "error: could not open ofstream" << endl;
       header.command = cxi_command::NAK;
    }
}

void reply_get (accepted_socket& client_sock, cxi_header& header) {
   outlog << "header.filename is " << header.filename << endl;
   std::ifstream is (header.filename, std::ifstream::binary);
      if (is) {
         header.command = cxi_command::FILEOUT;
         is.seekg (0, is.end);
         size_t length = is.tellg();
         is.seekg (0, is.beg);
         // don't think it needs to be length+1 bc just passing wo null
         auto buffer = make_unique<char[]>(length);
         is.read(buffer.get(), length);  
         // header.nbytes = htonl(sizeof buffer);
         header.nbytes = htonl(length);
         // 1
         outlog << "sending header " << header << endl;
         send_packet (client_sock, &header, sizeof header);
         // 2
         cout << "sending buffer" << endl;
         // sizeof buffer works, host_nbytes doesn't
         send_packet(client_sock, buffer.get(), sizeof buffer);  
         outlog << "sent buffer" << endl;
         is.close();
      }
      else {  // File doesn't exist, so send NAK
         outlog << "error: could not find file" << endl;
         header.command = cxi_command::NAK;
      }
}

// void reply_get (accepted_socket& client_sock, cxi_header& header) {
//    outlog << "header.filename is " << header.filename << endl;
//    std::ifstream is (header.filename, std::ifstream::binary);
//       if (is) {
//          is.seekg (0, is.end);
//          // size_t length = is.tellg();
//          // auto host_nbytes = htonl(host_size);   // change to network endianness
//          int length = is.tellg();
//          header.command = cxi_command::FILEOUT;

//          size_t host_nbytes = htonl(length);
//          auto buffer = make_unique<char[]>(length+1); // maybe wo +1
//          is.read(buffer.get(), length);  

//          cout << "host_nbytes" << host_nbytes << endl;
//          cout << "sizeof (buffer) is " << sizeof buffer << endl;
//          host_nbytes = htonl(sizeof buffer);
//          header.nbytes = host_nbytes;
//          is.seekg (0, is.beg);

//          // 1
//          outlog << "sending header " << header << endl;
//          send_packet (client_sock, &header, sizeof header);

//          // 2
//          cout << "sending buffer" << endl;
//          // sizeof buffer works, host_nbytes doesn't
//          send_packet(client_sock, buffer.get(), sizeof buffer);  
//          outlog << "sent buffer" << endl;

//          is.close();
//       }
//       else {  // File doesn't exist, so send NAK
//          outlog << "error: could not find file" << endl;
//          header.command = cxi_command::NAK;
//       }
// }

void reply_rm (accepted_socket& client_sock, cxi_header& header) {
   int status = unlink(header.filename);  // Delete from filesys
   if(status == 0) {
      header.command = cxi_command::ACK;
      outlog << "deleted the file. Sending header " << header << endl;
   }
   else {
      header.command = cxi_command::NAK;
      outlog << "could not find file. Sending header " << header << endl;
   }
   send_packet(client_sock, &header, sizeof header);
}


void run_server (accepted_socket& client_sock) {
   outlog.execname (outlog.execname() + "-server");
   outlog << "connected to " << to_string (client_sock) << endl;
   try {   
      for (;;) {
         cxi_header header; 
         // 0
         recv_packet (client_sock, &header, sizeof header);
         outlog << "received header " << header << endl;
         switch (header.command) {
            case cxi_command::LS: 
               reply_ls (client_sock, header);
               break;
            case cxi_command::PUT: 
               outlog << "entering reply_put" << endl;
               reply_put (client_sock, header);
               break;
            case cxi_command::RM: 
               reply_rm (client_sock, header);
               break;
            case cxi_command::GET: 
               reply_get (client_sock, header);
               break;
            default:
               outlog << "invalid client header:" << header << endl;
               break;
         }
      }
   }catch (socket_error& error) {
      outlog << error.what() << endl;
   }catch (cxi_exit& error) {
      outlog << "caught cxi_exit" << endl;
   }
   outlog << "finishing" << endl;
   throw cxi_exit();
}

void fork_cxiserver (server_socket& server, accepted_socket& accept) {
   pid_t pid = fork();
   if (pid == 0) { // child
      server.close();
      run_server (accept);
      throw cxi_exit();
   }else {
      accept.close();
      if (pid < 0) {
         outlog << "fork failed: " << strerror (errno) << endl;
      }else {
         outlog << "forked cxiserver pid " << pid << endl;
      }
   }
}


void reap_zombies() {
   for (;;) {
      int status;
      pid_t child = waitpid (-1, &status, WNOHANG);
      if (child <= 0) break;
      outlog << "child " << child
             << " exit " << (status >> 8)
             << " signal " << (status & 0x7F)
             << " core " << (status >> 7 & 1) << endl;
   }
}

void signal_handler (int signal) {
   outlog << "signal_handler: caught " << strsignal (signal) << endl;
   reap_zombies();
}

void signal_action (int signal, void (*handler) (int)) {
   struct sigaction action;
   action.sa_handler = handler;
   sigfillset (&action.sa_mask);
   action.sa_flags = 0;
   int rc = sigaction (signal, &action, nullptr);
   if (rc < 0) outlog << "sigaction " << strsignal (signal)
                      << " failed: " << strerror (errno) << endl;
}


int main (int argc, char** argv) {
   outlog.execname (basename (argv[0]));
   outlog << "starting" << endl;
   vector<string> args (&argv[1], &argv[argc]);
   signal_action (SIGCHLD, signal_handler);
   in_port_t port = get_cxi_server_port (args, 0);
   try {
      server_socket listener (port);
      for (;;) {
         outlog << to_string (hostinfo()) << " accepting port "
             << to_string (port) << endl;
         accepted_socket client_sock;
         for (;;) {
            try {
               listener.accept (client_sock);
               break;
            }catch (socket_sys_error& error) {
               switch (error.sys_errno) {
                  case EINTR:
                     outlog << "listener.accept caught "
                         << strerror (EINTR) << endl;
                     break;
                  default:
                     throw;
               }
            }
         }
         outlog << "accepted " << to_string (client_sock) << endl;
         try {
            fork_cxiserver (listener, client_sock);
            reap_zombies();
         }catch (socket_error& error) {
            outlog << error.what() << endl;
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


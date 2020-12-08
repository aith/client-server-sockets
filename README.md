## A client-server terminal interface
cxi - client  
cxid - server (d for daemon)  

### Usage
```
make
mkdir local
mkdir remote
ln -s cxid remote/cxid
ln -s cxi local/cxi
```
In one terminal, run the local folder's cxi  
in another, run the remote folder's cxid

### Commands 
```
ls    # list files
put   # put file into server
get   # get file from server
rm    # remove file from server
exit  # exit
```

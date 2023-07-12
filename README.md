# Server_Client_Socket_Programming

## Overview
- Provide the file, the IP address, and the port number you want to connect(Server) to as command-line arguments to the client program.
- The client attempts to establish a socket connection with the Server and sends the file.
- The Server program receives the code file from Client, compiles it using the gcc command, and executes it.
- The client program's stdin and stdout should appear as if they are interacting with the stdin and stdout of the executed program (in Server):

  a. Redirect the server's stdin and stdout to the client socket stream.

  b. Send the input from the client to the serverand output the received content from the server program.

## Requirement

#### Client Program
- 1. Command the IP address, port number, and file path for arguments in client program.
- 2. Sends the code file to the server.
- 3. Outputs the execution result of the transmitted code on the server side.
- 4. Passes input to the server program.
- 5. Terminates the program when receiving EOF from the server.

#### Server Program
- 1. As the server program needs to function as a backend, it should be able to establish and release connections with multiple clients.
- 2. Upon establishing a connection with a client, the server program saves the received code file to an arbitrary path (or the path where the server program is located) and performs the gcc compile command on that file using the system function.
- 3. From the moment the client program is connected, all messages output to stdout are redirected to the client-side socket.
- 4. All input received from the client program is redirected to stdin.
- 5. After disconnecting from the client, the file descriptor that was redirected is restored to its original state, ensuring that system messages are correctly output from the server.
- 6. Once the program executed on the server side is finished, the server sends EOF to the client and returns to the state of waiting for a new connection.

## Example
- Execution of the original program
  ```
  ## program execution
  $ ./code
  
  ## input number
  1 2
  result: 3 ## result
  
  ## return to the console after program termination
  $_
  ```
- Client
  ```
  $ ./client 192.168.0.2 24210 code.c
  
  ## input on the console after establishing connection with the server
  1 2

  ## the executed content is redirected and transmitted
  [remote] result: 3

  ## received EOF after the executed program finishes
  [*] session closed

  ## return to the console again
  $_

  ```

- Server
  ```
  $ ./server
  
  ## waiting for client connection
  [*] wait for client ...
  [*] client connected

  ## currently, the received code is being executed
  ## the content that should have been originally displa

  ## when the executed program finishes
  [*] session closed

  ## waiting again
  [*] wait for client ...

  ```


## Appendix
- `System()` :The system() function allows executing a command similar to directly entering a string in the Shell. Internally, it is composed of functions like fork() and exec(), and it uses the same standard streams (stdin, stdout, stderr) as the process that called the system() function.
  ```
  #include <stdlib.h>
  int system(const char *string);
  ```
- `Redirection` : In computing, redirection is a form of interprocess communication, and is a function common to most command-line interpreters, including the various Unix shells that can redirect standard streams to user-specified locations.
Briefly redirection refers to redirecting standard streams such as stdin and stdout to different directions for output. There are various ways to specify a different direction, and you can solve it based on what you have learned in the practice.


| Purpose                 | POSIX (Linux, macOS, Unix)   | Windows                                           |
|-------------------------|------------------------------|---------------------------------------------------|
| **Networking**          | `<sys/socket.h>`             | `<winsock2.h>`                                    |
|                         | `<netinet/in.h>`             | `<ws2tcpip.h>`                                    |
|                         | `<arpa/inet.h>`              | `<windows.h>`                                     |
|                         | `<unistd.h>`                 | `<io.h>`                                          |
|                         | `<netdb.h>`                  | `<ws2tcpip.h>`                                    |
|                         | `<sys/types.h>`              |                                                   |
| **File I/O**            | `<fcntl.h>`                  | `<io.h>`                                          |
|                         | `<sys/stat.h>`               | `<direct.h>`                                      |
|                         | `<dirent.h>`                 | `<windows.h>`                                     |
|                         | `<stdio.h>`                  |                                                   |
| **Threads**             | `<pthread.h>`                | `<windows.h>` (for CreateThread)                  |
|                         | `<semaphore.h>`              | `<synchapi.h>`                                    |
|                         | `<time.h>`                   |                                                   |
| **Memory Management**   | `<stdlib.h>`                 | `<stdlib.h>`                                      |
|                         | `<string.h>`                 | `<string.h>`                                      |
| **Timers**              | `<time.h>`                   | `<windows.h>` (for Sleep, etc.)                   |
|                         | `<sys/time.h>`               |                                                   |
| **Dynamic Loading**     | `<dlfcn.h>`                  | `<windows.h>` (for LoadLibrary)                   |
| **Signals**             | `<signal.h>`                 | `<windows.h>` (for structured exception handling) |
| **Process Control**     | `<sys/types.h>`              | `<process.h>`                                     |
|                         | `<sys/wait.h>`               |                                                   |
| **Environment**         | `<stdlib.h>`                 | `<process.h>` (for \_getenv, \_putenv)            |
|                         | `<unistd.h>`                 |                                                   |
| **Terminal I/O**        | `<termios.h>`                | `<conio.h>`                                       |
|                         |                              | `<windows.h>` (for console APIs)                  |
| **Graphics (GUI)**      | `<X11/Xlib.h>` (X11)         | `<windows.h>` (GDI, Direct2D)                     |
|                         | `<GL/gl.h>` (OpenGL)         | `<gl/gl.h>` (OpenGL)                              |
|                         | `<SDL2/SDL.h>` (SDL library) | `<SDL.h>` (if using SDL)                          |
| **Regular Expressions** | `<regex.h>`                  | `<regex>` (from C++ Standard Library)             |
| **Cryptography**        | `<openssl/ssl.h>`            | `<wincrypt.h>`                                    |
| **Compression**         | `<zlib.h>`                   | `<zlib.h>`                                        |
| **Multimedia**          | `<AL/al.h>` (OpenAL)         | `<al.h>` (OpenAL)                                 |
| **JSON Handling**       | `<json-c/json.h>`            | `<json-c/json.h>`                                 |
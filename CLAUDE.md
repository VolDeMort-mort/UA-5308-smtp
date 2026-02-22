# About
This is a project for implementing SMTP protocol suite (both user and network infrastructure side). It is an educational one, the goal is to get some practice with computer networking and C++ development in general. We mainly stick with RFC 821 as a reference, but also have to implement some extensions from later revisions, albeit in a loose sense (for example, see [encryption](#encryption)).
We have divided the project into following zones of responsibilities:
- Utilities: base64 functions, procedures for parsing server config, etc.;
- SMTP Protocol Layer;
- Network Layer: provide abstractions and primitives to be used by upper layers;
- Encryption: end-to-end encryption;
- Concurrency: the project is written in C++ so this group is responsible for providing higher level primitives that C++ lacks;
- Database integration: schema design (both client and a server);
- IMAP implementation;
- User agent graphical interface;

This division is described in more detail on GitHub board of this project, so for current task status consult that via github cli (if available) or web search facilities

The project is in very early stage so frequent updates to this file to reflect project state are encouraged. Statements that are true as of time of the writing but meant to change are marked as incomplete tasks using markdown todo syntax and as such have to be inspected thoroughly as we move forward.

## Project layout and build instructions

- proto subdirectory hosts shared definitions between client and a server;
- subtasks implementing functionality that otherwise would be pulled as a library (logger, utilities) have their own subdirectory and have to be built as a vendored library (respective CMake files should always reflect this intent);
- [ ] developers of various subsystems should be able to run and test their code even without functionality they dependend upon being ready. This is already done compilation-wise via BUILD_{SUBSYSTEM} options but has to be extended to runtime via stubs or other appropriate mechanism;

## Subtasks

### Encryption

We have to roll our own encryption that is in spirit TLS but does not concern itself with certificates and other related complexities. It should demonstrate the gist of how shared secred for secure communication is negotiated over non encrypted medium, leaving the actual math machinery to a library (in our case, libsodium).

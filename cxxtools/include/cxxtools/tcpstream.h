////////////////////////////////////////////////////////////////////////
// tcpstream.h
//
// Author: Tommi M�kitalo, Dr. Eckhardt + Partner GmbH
//
#ifndef TCPSTREAM_H
#define TCPSTREAM_H

#include <iostream>
#include <string>
#include <stdexcept>
#include <netinet/in.h>

namespace tcp
{
  //////////////////////////////////////////////////////////////////////
  /**
   * tcp::Exception wird bei Fehlern ausgel�st.
   */
  class Exception : public std::runtime_error
  {
    public:
      /// Konstruktor erzeugt eine Instanz mit einer Fehlernummer
      /// und einer Fehlermeldung.
      Exception(int Errno, const std::string& msg);
      /// Konstruktor erzeugt eine Instanz mit einer Fehlermeldung.
      /// Die Nummer wird aus errno �bernommen.
      explicit Exception(const std::string& msg);

      /// Liefert die Fehlernummer zur�ck.
      int getErrno() const
      { return m_Errno; }

    private:
      int m_Errno;
  };

  //////////////////////////////////////////////////////////////////////
  /**
   * tcp::Timeout wird bei Zeit�berschreitung ausgel�st.
   */
  class Timeout : public Exception
  {
    public:
      /// Konstruktor erzeugt eine Instanz mit einer Fehlermeldung.
      Timeout()
        : Exception(0, "Timeout")
      { }
  };

  //////////////////////////////////////////////////////////////////////
  /**
   * Die Klasse wird geworfen, wenn keine Zeichen gelesen werden konnten.
   */
  class eof : public std::exception
  {
    public:
      /// erzeugt eine Instanz
      eof()  { }
      /// Liefert einfache Meldung zur�ck
      const char* what() const throw()
      { return "EOF"; }
  };

  //////////////////////////////////////////////////////////////////////
  /**
   * Basisklasse f�r Sockets
   */
  class Socket
  {
    public:
      /// Konstruktor erzeugt eine Instanz und erzeugt einen Socket
      /// auf Betriebssystemebene. Bei Fehlern wird eine Exception
      /// ausgel�st.
      Socket(int domain, int type, int protocol) throw (Exception);

      /// Konstruktor initialisiert die Klasse mit einem bestehenden
      /// Socket.
      explicit Socket(int fd = -1)
        : m_sockFd(fd)
        { }

      /// Im Destruktor wird der Socket geschlossen, wenn er offen ist.
      /// Bei einem Fehler wird eine Fehlermeldung auf stderr geschrieben.
      virtual ~Socket();

      /// Liefert true zur�ck, wenn der Socket ge�ffnet ist
      bool good() const     { return m_sockFd >= 0; }
      /// Liefert true zur�ck, wenn der Socket nicht ge�ffnet ist
      bool bad() const      { return m_sockFd <  0; }
      /// Liefert true zur�ck, wenn der Socket ge�ffnet ist
      operator bool() const { return m_sockFd >= 0; }

      /// Erzeugt einen Socket auf Betriebssystemebene. Ist bereits einer
      /// ge�ffnet, wird dieser vorher geschlossen. Bei Fehlern wird eine
      /// Exception ausgel�st.
      void create(int domain, int type, int protocol) throw (Exception);
      /// Schie�t den Socket, wenn er ge�ffnet ist.
      void close();

      /// Liefert den Sockethandle zur�ck
      int getFd() const     { return m_sockFd; }

    protected:
      void setFd(int sockFd)
      {
        close();
        m_sockFd = sockFd;
      }

    private:
      int      m_sockFd;

      Socket(const Socket&);               // no implementation
      Socket& operator= (const Socket&);   // no implementation
  };

  //////////////////////////////////////////////////////////////////////
  /**
   * Serversocket
   */
  class Server : public Socket
  {
    public:
      /// erzeugt einen Serversocket und ruft listen auf
      explicit Server(int backlog = 5) throw (Exception);
      /// erzeugt einen Serversocket und h�rt auf die angegebene Adresse
      Server(const std::string& ipaddr, int port, int backlog = 5)
             throw (Exception);
      /// erzeugt einen Serversocket und h�rt auf die angegebene Adresse
      Server(const char* ipaddr, int port, int backlog = 5) throw (Exception);

      void Listen(int backlog) throw (Exception);
      void Listen(const char* ipaddr, int port, int backlog) throw (Exception);

    private:
  };

  //////////////////////////////////////////////////////////////////////
  /**
   * Stream repr�sentiert ein Client-Socket.
   *
   * Stream ist ein ungepufferter Client-Socket zum lesen und schreiben
   * von Daten �ber den Socket.
   */
  class Stream : public Socket
  {
      socklen_t peeraddr_len;
      union {
        struct sockaddr sockaddr;
        struct sockaddr_in sockaddr_in;
      } peeraddr;

    public:
      typedef size_t size_type;

      /// initialisert einen leeren Socket
      Stream();
      /// nimmt eine Verbindung von einem Serversocket an.
      explicit Stream(const Server& server);
      /// Initialisert einen Socket und nimmt Verbindung mit der angegebenen
      /// Adresse auf. Kommt keine Verbindung zustande, wird eine Exception
      /// ausgel�st
      Stream(const std::string& ipaddr, int port);
      /// Initialisert einen Socket und nimmt Verbindung mit der angegebenen
      /// Adresse auf. Kommt keine Verbindung zustande, wird eine Exception
      /// ausgel�st
      Stream(const char* ipaddr, int port);
      /// Intantiiert ein Stream mit einem exisitierenden Filedeskriptor.
      /// Die Klasse �bernimmt den Besitz des Deskriptors und schliesst diesen
      /// im Destruktor, wenn er >= 0 ist.
      explicit Stream(int fd);

      /// akzeptiert eine Verbindung von einem Server. Kommt keine
      /// Verbindung zustande, wird eine Exception ausgel�st.
      void Accept(const Server& server);
      /// nimmt eine Verbindung mit der angegebenen Adresse auf. Kommt keine
      /// Verbindung zustande, wird eine Exception ausgel�st.
      void Connect(const char* ipaddr, int port);
      /// nimmt eine Verbindung mit der angegebenen Adresse auf. Kommt keine
      /// Verbindung zustande, wird eine Exception ausgel�st.
      void Connect(const std::string& ipaddr, int port)
      { Connect(ipaddr.c_str(), port); }

      /// Liest vom Socket maximal 'bufsize' Zeichen in 'buffer'.
      /// Liefert die Anzahl der gelesenen Zeichen zur�ck.
      /// Bei Fehler wird eine tcp::Exception geworfen.
      /// timeout in Millisekunden
      size_type Read(char* buffer, size_type bufsize, int timeout = -1) const;
      /// Schreibt bis zu 'bufsize' Zeichen aus 'buffer' auf den Socket.
      /// Liefert die Anzahl der geschriebenen Zeichen zur�ck.
      size_type Write(const char* buffer, size_type bufsize) const;

      /// gibt die aktuelle Peer-Adresse zur�ck
      const struct sockaddr& getPeeraddr() const
        { return peeraddr.sockaddr; }
      const struct sockaddr_in& getPeeraddr_in() const
        { return peeraddr.sockaddr_in; }
  };

  //////////////////////////////////////////////////////////////////////
  /**
   * streambuf ist ein std::streambuf f�r Socket-Verbindungen.
   *
   * Die Klasse wird von tcp::iostream verwendet, um die Pufferung
   * zu realisieren.
   */
  class streambuf : public std::streambuf
  {
#if __GNUC__ <= 2
    typedef int int_type;
    typedef char char_type;

    class traits_type
    {
      public:
        static int eof()
        { return EOF; }
    };

#endif

    public:
      /// Konstuktor reserviert Speicher f�r den E/A-Puffer
      explicit streambuf(Stream& stream, unsigned bufsize = 256,
        int timeout = -1);

      /// Destruktor gibt Speicher frei
      ~streambuf()
      { delete[] m_buffer; }

      void setTimeout(int t)   { m_timeout = t; }
      int getTimeout() const   { return m_timeout; }

      /// �berladen aus std::streambuf
      int_type overflow(int_type c);
      /// �berladen aus std::streambuf
      int_type underflow();
      /// �berladen aus std::streambuf
      int sync();

    private:
      Stream&    m_stream;
      unsigned   m_bufsize;
      char_type* m_buffer;
      int        m_timeout;
  };

  //////////////////////////////////////////////////////////////////////
  /**
   * Stream-Socket erweitert um Pufferung �ber std::iostream-Bibliothek
   */
  class iostream : public std::iostream, public Stream
  {
    public:
      /// Erzeugt einen nicht verbundenen Stream-Socket.
      explicit iostream(unsigned bufsize = 256, int timeout = -1)
        : std::iostream(&m_buffer),
          Stream(-1),
          m_buffer(*this, bufsize, timeout)
        { }

      /// Nimmt eine eingehende Verbindung vom angegebenen Server an.
      explicit iostream(const Server& server, unsigned bufsize = 256)
        : std::iostream(&m_buffer),
          Stream(server),
          m_buffer(*this, bufsize)
        { }

      /// Baut eine Verbindung zum angegebenen Server auf.
      iostream(const char* ipaddr, int port, unsigned bufsize = 256)
        : std::iostream(&m_buffer),
          Stream(ipaddr, port),
          m_buffer(*this, bufsize)
        { }

      /// Baut eine Verbindung zum angegebenen Server auf.
      iostream(std::string ipaddr, int port, unsigned bufsize = 256)
        : std::iostream(&m_buffer),
          Stream(ipaddr, port),
          m_buffer(*this, bufsize)
        { }

      void setTimeout(int timeout)  { m_buffer.setTimeout(timeout); }
      int getTimeout() const        { return m_buffer.getTimeout(); }

    private:
      streambuf m_buffer;
  };

} // namespace tcp

#endif // TCPSTREAM_H

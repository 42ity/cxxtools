////////////////////////////////////////////////////////////////////////
// sudo.h
//

#ifndef SUDO_H
#define SUDO_H

#include <sys/types.h>
#include <utility>

namespace sudo
{
  void set_user(uid_t uid);
  void set_user(const char* user);

  //////////////////////////////////////////////////////////////////////
  // sudo::fork_wait
  //
  // f�hrt einen ::fork aus.
  // Im Child-Prozess wird im ersten R�ckgabewert (first) 'false'
  // geliefert.
  // Der Parent-Prozess wartet auf den Child-Prozess und kehrt erst zur�ck,
  // wenn der Child-Prozess beendet ist. Der erste R�ckgabewert ist 'true'.
  // Der zweite R�ckgabewert (second) liefert den Status des
  // Childprozesses.
  //
  // Im Fehlerfall wird eine std::runtime_error geworfen.
  //
  std::pair<bool, int> fork_wait();

  //////////////////////////////////////////////////////////////////////
  // sudo f�hrt die �bergebene Funktion, bzw. den Funktor unter der
  // angegebenen Benutzerberechtigung aus.
  // Der Benutzer wird �ber die Funktion set_user eingestellt und kann
  // so entweder als const char* oder uid_t �bergeben werden.
  // Im Fehlerfall wird eine std::runtime_error geworfen.
  //
  // Der Funktor liefert den R�ckgabewert.
  //
  template <typename user_type, typename generator_type>
  int sudo(user_type user, generator_type f)
  {
    std::pair<bool, int> ret = fork_wait();
    if (!ret.first)
    {
      // child
      try
      {
        set_user(user);
        exit( f() );
      }
      catch (...)
      {
        exit(-1);
      }
    }

    // parent
    return ret.second;
  }

}

#endif // SUDO_H

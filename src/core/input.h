#pragma once
#include "type_alias.h"

class Input
{
public:
  Input();

  uint8 read() const;
  void write(const uint8 value);

private:
  uint8 m_p1;
  const bool* m_inputBuffer;
};

#pragma once
#include "type_alias.h"
#include <vector>
#include <array>
#include <filesystem>

class Cartridge;
class CartridgeSlot
{
public:
	CartridgeSlot();
	~CartridgeSlot();
	
	//poi vediamo
	std::string getCartridgeName() const;

	void reset();
	void loadCartridge(const std::filesystem::path& filePath);
	void reloadCartridge();
	bool hasCartridge() const;
	
	uint8 readRom(const uint16 addr) const;
	void writeRom(const uint16 addr, const uint8 value);
	uint8 readRam(const uint16 addr) const;
	void writeRam(const uint16 addr, const uint8 value);
private:
	Cartridge* m_cartridge;
};
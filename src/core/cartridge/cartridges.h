#pragma once
#include "type_alias.h"
#include <vector>
#include <filesystem>

class Cartridge
{
public:
    Cartridge(const std::filesystem::path& path, bool hasRam = false, bool hasBattery = false);
    virtual ~Cartridge() = default;
    
    void save();
    void loadSave();
    
    const std::filesystem::path& getPath() const;

    virtual uint8 readRom(const uint16 addr);
    virtual void writeRom(const uint16 addr, const uint8 value);
    virtual uint8 readRam(const uint16 addr);
    virtual void writeRam(const uint16 addr, const uint8 value);
protected:
    static constexpr uint16 kb2{0x800};
    static constexpr uint16 kb8{0x2000};
    static constexpr uint16 kb16{0x4000};

    std::filesystem::path m_path;
    std::vector<uint8> m_rom;
	std::vector<uint8> m_ram;
    bool m_hasRam;
    bool m_hasBattery;
    uint16 m_romBanks;
    uint16 m_ramBanks;
    
    bool m_externalRamEnabled;
    uint16 m_romBankIndex;
    uint16 m_ramBankIndex;
private:
    bool loadRom();
};

class CartridgeMbc1 : public Cartridge
{
public:
    CartridgeMbc1(const std::filesystem::path& path, bool hasRam = false, bool hasBattery = false);

    uint8 readRom(const uint16 addr) override;
    void writeRom(const uint16 addr, const uint8 value) override;
    uint8 readRam(const uint16 addr) override;
    void writeRam(const uint16 addr, const uint8 value) override;
private:
    uint32 m_ramSize;
    uint8 m_romBankIndexMask;
    bool m_modeFlag;
};

class CartridgeMbc3 : public Cartridge
{
public:
    uint8 readRom(const uint16 addr) override;
    void writeRom(const uint16 addr, const uint8 value) override;
    uint8 readRam(const uint16 addr) override;
    void writeRam(const uint16 addr, const uint8 value) override;
};

class CartridgeMbc5 : public Cartridge
{
public:
    CartridgeMbc5(const std::filesystem::path& path, bool hasRam = false, bool hasBattery = false, bool hasRumble = false);

    uint8 readRom(const uint16 addr) override;
    void writeRom(const uint16 addr, const uint8 value) override;
    uint8 readRam(const uint16 addr) override;
    void writeRam(const uint16 addr, const uint8 value) override;
private: 
    bool m_hasRumble;
};
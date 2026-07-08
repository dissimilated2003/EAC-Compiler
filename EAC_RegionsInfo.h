#ifndef EAC_REGIONS_INFO
#define EAC_REGIONS_INFO


#include <memory>
#include <string>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <iostream>


// перечисление регионов, в которых отличается вёрстка
enum class Region
{
    BR_PT,     
    EN,        
    DACH,       
    ES_LATAM,  
    FR,        
    IT,        
    JP,        
    VN,

    UNKNOWN
};


// структура, хранящая региональную информацию. создаётся фабрикой
struct RegionData
{
    std::string headerLink;
    std::string facebookLink;
    std::string xLink;
    std::string linkedInLink;
    std::string youTubeLink;
    std::string instagramLink;
    std::string aboutiSpringLink;
    std::string privacyPolicyLink;
    std::string aboutiSpringText;
    std::string unsubscribeText;
    std::string privacyPolicyText;
};


// интерфейс "Информация о регионе"
// задаёт контракт обязательной информации для письма
// реализации - разные регионы, у которых отличается текст ссылок и сами ссылки
// при отсутствии ссылки на ресурс функции возвращают "" или дефолтные EN-ссылки
struct IRegionInfo
{
    
    virtual std::string GetHeaderLink()        const = 0; // iSpring logo
    virtual std::string GetFacebookLink()      const = 0; 
    virtual std::string GetXLink()             const = 0; 
    virtual std::string GetLinkedInLink()      const = 0; 
    virtual std::string GetYouTubeLink()       const = 0;
    virtual std::string GetInstagramLink()     const = 0;
    virtual std::string GetAboutiSpringLink()  const = 0;
    virtual std::string GetPrivacyPolicyLink() const = 0;
    virtual std::string GetAboutiSpringText()  const = 0;
    virtual std::string GetUnsubscribeText()   const = 0;
    virtual std::string GetPrivacyPolicyText() const = 0;

    virtual ~IRegionInfo() = default;
};


// этот класс будет предоставляться клиенту (ICampaignKernel) и будет храниться 
// в его реализациях как уникальный указатель
class RegionInfo final : public IRegionInfo
{
public:
    RegionInfo() = default;
    explicit RegionInfo(RegionData regionData) : m_regionData(std::move(regionData)) {}

    std::string GetHeaderLink()        const override { return m_regionData.headerLink; }
    std::string GetFacebookLink()      const override { return m_regionData.facebookLink; }
    std::string GetXLink()             const override { return m_regionData.xLink; }
    std::string GetLinkedInLink()      const override { return m_regionData.linkedInLink; }
    std::string GetYouTubeLink()       const override { return m_regionData.youTubeLink; }
    std::string GetInstagramLink()     const override { return m_regionData.instagramLink; }
    std::string GetAboutiSpringLink()  const override { return m_regionData.aboutiSpringLink; }
    std::string GetPrivacyPolicyLink() const override { return m_regionData.privacyPolicyLink; }
    std::string GetAboutiSpringText()  const override { return m_regionData.aboutiSpringText; }
    std::string GetUnsubscribeText()   const override { return m_regionData.unsubscribeText; }
    std::string GetPrivacyPolicyText() const override { return m_regionData.privacyPolicyText; }
private:
    RegionData m_regionData;
};


// фабрика, порождающая региональную информацию
class RegionInfoFactory 
{
public:
    RegionInfoFactory() = default;

    void ParseRegion(const std::string& inRegion) // параметр приходит из argv[1]
    {
        std::string reg = inRegion;
        std::transform(reg.begin(), reg.end(), reg.begin(), [](unsigned char ch){ return std::tolower(ch); });

        if (reg == "en-emea" || reg == "anz" || reg == "anz-apac" || reg == "apac" || reg == "na" || reg == "en" ||
            reg == "see" || reg == "uki" || reg == "nordic" || reg == "nl" || reg == "za" || reg == "asia")
        {
            m_region = Region::EN;
        }
        else if (reg == "dach" || reg == "de")
        {
            m_region = Region::DACH;
        }
        else if (reg == "br-pt" || reg == "br" || reg == "pt")
        {
            m_region = Region::BR_PT;
        }
        else if (reg == "es" || reg == "es-latam" || reg == "latam")
        {
            m_region = Region::ES_LATAM;
        }
        else if (reg == "fr")
        {
            m_region = Region::FR;
        }
        else if (reg == "it")
        {
            m_region = Region::IT;
        }
        else if (reg == "jp")
        {
            m_region = Region::JP;
        }
        else if (reg == "vn")
        {
            m_region = Region::VN;
        }
        else
        {
            m_region = Region::UNKNOWN;
        }
    }
 
    std::unique_ptr<IRegionInfo> CreateRegionInfo() 
    {
        switch (m_region)
        {
            case Region::EN:        return CreateENRegionInfo();
            case Region::BR_PT:     return CreateBRPTRegionInfo();
            case Region::DACH:      return CreateDACHRegionInfo();
            case Region::ES_LATAM:  return CreateESLATAMRegionInfo();
            case Region::FR:        return CreateFRRegionInfo();
            case Region::IT:        return CreateITRegionInfo();
            case Region::JP:        return CreateJPRegionInfo();
            case Region::VN:        return CreateVNRegionInfo();
            case Region::UNKNOWN:   
                PrintAvailableRegionTokens();
                throw std::invalid_argument("Unknown region");
            default: 
                throw std::invalid_argument("Unknown region");
        }
    }

    Region GetRegion() const 
    {
        return m_region;
    }

private:
    Region m_region = Region::UNKNOWN;

    // при вводе неверного региона всегда будет вылетать справка с доступными регионами
    void PrintAvailableRegionTokens() 
    {
        std::cout << "Available regions:\n";
        std::cout << "English:    NA, EN-EMEA, ANZ, ANZ-APAC, APAC, EN, SEE, UKI, Nordic, NL, ZA, ASIA\n";
        std::cout << "German:     DACH, DE\n";
        std::cout << "Brazil:     BR, PT, BR-PT\n";
        std::cout << "Spanish:    ES, ES-LATAM, LATAM\n";
        std::cout << "French:     FR\n";
        std::cout << "Italian:    IT\n";
        std::cout << "Japanese:   JP\n";
        std::cout << "Vietnamese: VN\n";
        std::cout << "Use any of these tokens as the first argument\n";
    }

    std::unique_ptr<IRegionInfo> CreateENRegionInfo() 
    {
        return std::make_unique<RegionInfo>(RegionData{
            .headerLink         = "https://www.ispringsolutions.com/",
            .facebookLink       = "https://www.facebook.com/iSpringPro",
            .xLink              = "https://x.com/iSpringPro",
            .linkedInLink       = "https://ru.linkedin.com/company/ispring-solutions",
            .youTubeLink        = "https://www.youtube.com/user/iSpringPro",
            .instagramLink      = "https://www.instagram.com/ispringsolutions/",
            .aboutiSpringLink   = "https://www.ispringsolutions.com/",
            .privacyPolicyLink  = "https://www.ispringsolutions.com/company/policy/privacy",
            .aboutiSpringText   = "About iSpring",
            .unsubscribeText    = "Unsubscribe",
            .privacyPolicyText  = "Privacy Policy",
        });
    }

    std::unique_ptr<IRegionInfo> CreateBRPTRegionInfo()
    {
        return std::make_unique<RegionInfo>(RegionData{
            .headerLink         = "https://www.ispringpro.com.br",
            .facebookLink       = "https://www.facebook.com/ispring.br",
            .xLink              = "https://x.com/iSpringPro",
            .linkedInLink       = "https://www.linkedin.com/company/ispring-brasil/",
            .youTubeLink        = "https://www.youtube.com/@iSpringBR/",
            .instagramLink      = "https://www.instagram.com/ispring.brasil/",
            .aboutiSpringLink   = "https://www.ispringpro.com.br",
            .privacyPolicyLink  = "https://www.ispringpro.com.br/company/policy/privacy",
            .aboutiSpringText   = "Sobre a iSpring",
            .unsubscribeText    = "Descadastrar",
            .privacyPolicyText  = "Política de Privacidade",
        });
    }

    std::unique_ptr<IRegionInfo> CreateDACHRegionInfo()
    {
        return std::make_unique<RegionInfo>(RegionData{
            .headerLink         = "https://www.ispringlearn.de/",
            .facebookLink       = "https://www.facebook.com/iSpringDE/",
            .xLink              = "https://x.com/iSpringPro",
            .linkedInLink       = "https://www.linkedin.com/company/77171159/",
            .youTubeLink        = "https://www.youtube.com/channel/UCghzcZx8UGnTtCJ0i85SOLw",
            .instagramLink      = "https://www.instagram.com/ispringsolutions/",
            .aboutiSpringLink   = "https://www.ispringlearn.de/",
            .privacyPolicyLink  = "https://www.ispringlearn.de/datenschutz",
            .aboutiSpringText   = "Über iSpring",
            .unsubscribeText    = "Abmelden",
            .privacyPolicyText  = "Datenschutzerklärung",
        });
    }

    std::unique_ptr<IRegionInfo> CreateESLATAMRegionInfo()
    {
        return std::make_unique<RegionInfo>(RegionData{
            .headerLink         = "https://www.ispring.es/",
            .facebookLink       = "https://www.facebook.com/profile.php?id=100085684523216&sk=grid",
            .xLink              = "https://x.com/iSpringPro",
            .linkedInLink       = "https://www.linkedin.com/company/ispring-es",
            .youTubeLink        = "https://www.youtube.com/@iSpringES",
            .instagramLink      = "https://www.instagram.com/ispring.es",
            .aboutiSpringLink   = "https://www.ispring.es/",
            .privacyPolicyLink  = "https://www.ispring.es/politica-de-privacidad",
            .aboutiSpringText   = "Sobre iSpring",
            .unsubscribeText    = "Darse de baja",
            .privacyPolicyText  = "Política de Privacidad",
        });
    }

    std::unique_ptr<IRegionInfo> CreateFRRegionInfo()
    {
        return std::make_unique<RegionInfo>(RegionData{
            .headerLink         = "https://www.ispring.fr/",
            .facebookLink       = "https://www.facebook.com/iSpringPro",
            .xLink              = "https://x.com/iSpringPro",
            .linkedInLink       = "https://www.linkedin.com/company/89609234/",
            .youTubeLink        = "https://www.youtube.com/channel/UCkEsGLqeHEeS0iR5l76t5xg",
            .instagramLink      = "https://www.instagram.com/ispringsolutions/",
            .aboutiSpringLink   = "https://www.ispring.fr/",
            .privacyPolicyLink  = "https://www.ispring.fr/politique-de-confidentialite",
            .aboutiSpringText   = "À propos",
            .unsubscribeText    = "Désinscription",
            .privacyPolicyText  = "Politique de confidentialité",
        });
    }

    std::unique_ptr<IRegionInfo> CreateITRegionInfo()
    {
        return std::make_unique<RegionInfo>(RegionData{
            .headerLink         = "https://www.ispring.it",
            .facebookLink       = "https://www.facebook.com/iSpringPro",
            .xLink              = "https://x.com/iSpringPro",
            .linkedInLink       = "https://www.linkedin.com/company/ispring-italy",
            .youTubeLink        = "https://www.youtube.com/channel/UCWStKGPSHnFpFK1t6l_Aj9A",
            .instagramLink      = "https://www.instagram.com/ispringsolutions/",
            .aboutiSpringLink   = "https://www.ispring.it",
            .privacyPolicyLink  = "https://www.ispring.it/company/policy/privacy",
            .aboutiSpringText   = "Prodotti iSpring",
            .unsubscribeText    = "Cancellare l'iscrizion",
            .privacyPolicyText  = "Informativa sulla privacy",
        });
    }

    std::unique_ptr<IRegionInfo> CreateJPRegionInfo()
    {
        return std::make_unique<RegionInfo>(RegionData{
            .headerLink         = "https://www.ispring.jp",
            .facebookLink       = "https://www.facebook.com/iSpringPro",
            .xLink              = "https://x.com/iSpringPro",
            .linkedInLink       = "https://www.linkedin.com/companies/ispring-solutions",
            .youTubeLink        = "https://www.youtube.com/user/iSpringPro",
            .instagramLink      = "https://www.instagram.com/ispringsolutions/",
            .aboutiSpringLink   = "https://www.ispring.jp",
            .privacyPolicyLink  = "https://www.ispringsolutions.com/company/policy/privacy",
            .aboutiSpringText   = "iSpringについて",
            .unsubscribeText    = "登録解除",
            .privacyPolicyText  = "プライバシーポリシー",
        });
    }

    std::unique_ptr<IRegionInfo> CreateVNRegionInfo()
    {
        return std::make_unique<RegionInfo>(RegionData{
            .headerLink         = "https://www.ispringsolutions.com",
            .facebookLink       = "https://www.facebook.com/iSpringPro",
            .xLink              = "https://x.com/iSpringPro",
            .linkedInLink       = "https://www.linkedin.com/companies/ispring-solutions",
            .youTubeLink        = "https://www.youtube.com/user/iSpringPro",
            .instagramLink      = "https://www.instagram.com/ispringsolutions/",
            .aboutiSpringLink   = "https://www.ispringsolutions.com",
            .privacyPolicyLink  = "https://www.ispringsolutions.com/company/policy/privacy",
            .aboutiSpringText   = "Về iSpring",
            .unsubscribeText    = "Hủy đăng ký",
            .privacyPolicyText  = "Chính sách bảo mật",
        });
    }
};


#endif
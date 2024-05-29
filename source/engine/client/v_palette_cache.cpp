
#include <physfs.h>

#include <unordered_map>

#include "Poco/JSON/Parser.h"
#include "Poco/MemoryStream.h"
#include "i_system.h"
#include "libimagequant.h"
#include "m_fileio.h"
#include "stb_image.h"

using Poco::MemoryInputStream;
using Poco::JSON::Parser;

static std::string       image_directories[] = {"textures", "flats", "sprites"};
static constexpr int32_t cache_version       = 1;
static constexpr char   *cache_file          = "palette-cache.json";

class PaletteCache
{
  public:
    void process()
    {
        for (const std::string &dir : image_directories)
        {
            PHYSFS_enumerate(
                dir.c_str(),
                +[](void *data, const char *origdir, const char *fname) {
                    std::string path = std::string(origdir) + "/" + fname;
                    PHYSFS_Stat stat;
                    PHYSFS_stat(path.c_str(), &stat);
                    PaletteCache::mModTimes[path] = stat.modtime;
                    return PHYSFS_ENUM_OK;
                },
                nullptr);
        }

        if (!checkCache())
        {
            generatePalette();
        }
    }

    const std::vector<uint8_t> &getPalette()
    {
        if (mPalette.size() != 768)
        {
            mPalette.clear();
            for (int32_t i = 0; i < 768; i++)
            {
                mPalette.push_back(255);
            }
        }
        return mPalette;
    }

  private:
    struct ImageRGBAData
    {
        int32_t  width;
        int32_t  height;
        uint8_t *pixels;

        liq_attr  *handle;
        liq_image *image;
    };

    void generatePalette()
    {

        liq_attr      *histogram_attr = liq_attr_create();
        liq_histogram *histogram      = liq_histogram_create(histogram_attr);
        
        // Todo: see how this affects quality vs speed, though should be preprocessing palettes for distribution anyway
        // liq_set_speed(histogram_attr, 10);

        // load images
        std::vector<ImageRGBAData> images;

        for (auto itr : mModTimes)
        {
            const std::string &filename = itr.first;

            PHYSFS_File *imageFile = PHYSFS_openRead(filename.c_str());

            if (!imageFile)
            {
                I_Error("Unable to read image file %s", filename.c_str());
            }

            PHYSFS_sint64 filelen = PHYSFS_fileLength(imageFile);

            uint8_t *filedata = new uint8_t[filelen];

            if (PHYSFS_readBytes(imageFile, filedata, filelen) != filelen)
            {
                delete[] filedata;
                PHYSFS_close(imageFile);
                I_Error("PaletteCache::generatePalette: Error reading %s\n", filename.c_str());
            }

            PHYSFS_close(imageFile);

            int32_t       bpp;
            ImageRGBAData data;
            data.handle = nullptr;
            data.image  = nullptr;

            data.pixels = stbi_load_from_memory(filedata, filelen, &data.width, &data.height, &bpp, 4);

            if (!data.pixels)
            {
                delete[] filedata;
                I_Error("PaletteCache::generatePalette: Error loading %s\n", filename.c_str());
            }

            delete[] filedata;

            data.handle = liq_attr_create();
            data.image  = liq_image_create_rgba(data.handle, data.pixels, data.width, data.height, 0);

            liq_histogram_add_image(histogram, histogram_attr, data.image);

            images.push_back(data);
        }

        liq_result *res;
        liq_error   err = liq_histogram_quantize(histogram, histogram_attr, &res);

        if (err != LIQ_OK)
        {
            I_Error("PaletteCache::generatePalette: Error quantizing histogram");
        }

        const liq_palette *palette = liq_get_palette(res);

        mPalette.clear();
        for (int32_t i = 0; i < palette->count; i++)
        {
            mPalette.push_back(palette->entries[i].r);
            mPalette.push_back(palette->entries[i].g);
            mPalette.push_back(palette->entries[i].b);
        }

        for (int32_t i = palette->count; i < 256; i++)
        {
            mPalette.push_back(0);
            mPalette.push_back(0);
            mPalette.push_back(0);
        }

        liq_result_destroy(res);
        liq_histogram_destroy(histogram);
        liq_attr_destroy(histogram_attr);

        for (auto image : images)
        {
            liq_image_destroy(image.image);
            liq_attr_destroy(image.handle);
            free(image.pixels);
        }

        saveCache();
    }

    void saveCache()
    {
        Poco::JSON::Object cache;

        Poco::JSON::Object modTimes;

        for (auto itr : mModTimes)
        {
            modTimes.set(itr.first, itr.second);
        }

        cache.set("version", cache_version);

        cache.set("modTimes", modTimes);

        int32_t           idx = 0;
        Poco::JSON::Array jpalette;
        for (uint8_t c : mPalette)
        {
            jpalette.set(idx++, c);
        }

        cache.set("palette", jpalette);

        std::ostringstream json;
        cache.stringify(json, 4);

        PHYSFS_File *file = PHYSFS_openWrite(cache_file);
        if (!file)
        {
            I_Error("PaletteCache: Error opening %s for write", cache_file);
        }

        PHYSFS_writeBytes(file, json.str().c_str(), json.str().size());

        PHYSFS_close(file);
    }

    bool checkCache()
    {
        // load the current hashes if any

        if (!PHYSFS_exists(cache_file))
        {
            return false;
        }
        PHYSFS_File *file = PHYSFS_openRead(cache_file);

        if (!file)
        {
            I_Error("PaletteCache: Error opening %s", cache_file);
        }

        PHYSFS_sint64 size = PHYSFS_fileLength(file);
        if (size)
        {
            uint8_t *buffer = (uint8_t *)malloc(size);

            if (PHYSFS_readBytes(file, (void *)buffer, size) != size)
            {
                I_Error("PaletteCache: Error reading %s", cache_file);
            }

            MemoryInputStream contents((const char *)buffer, size);
            Parser            parser;
            parser.parse(contents);
            auto result = parser.result();

            free(buffer);
            PHYSFS_close(file);

            Poco::JSON::Object::Ptr obj = result.extract<Poco::JSON::Object::Ptr>();
            if (obj.isNull() || obj->getValue<int32_t>("version") != cache_version)
            {
                return false;
            }

            Poco::JSON::Object::Ptr modTimes = obj->getObject("modTimes");
            if (modTimes.isNull() || modTimes->size() != mModTimes.size())
            {
                return false;
            }

            for (auto itr : mModTimes)
            {
                if (itr.second != modTimes->getValue<PHYSFS_sint64>(itr.first))
                {
                    return false;
                }
            }

            Poco::JSON::Array::Ptr palette = obj->getArray("palette");
            if (palette.isNull() || palette->size() != 768)
            {
                return false;
            }

            mPalette.clear();

            for (int32_t i = 0; i < 768; i++)
            {
                mPalette.push_back(palette->get(i).convert<uint8_t>());
            }

            return true;
        }

        return false;
    }

    std::vector<uint8_t>                                  mPalette;
    static std::unordered_map<std::string, PHYSFS_sint64> mModTimes;
};

std::unordered_map<std::string, PHYSFS_sint64> PaletteCache::mModTimes;
static std::unique_ptr<PaletteCache>           g_palette_cache;

void V_PaletteCache_Init()
{
    if (g_palette_cache.get())
    {
        return;
    }

    g_palette_cache = std::make_unique<PaletteCache>();
    g_palette_cache->process();
}

void V_PaletteCache_GetPalette(std::vector<uint8_t> &palette)
{
    palette = g_palette_cache->getPalette();
}

void V_PaletteCache_Shutdown()
{
    g_palette_cache.reset();
}
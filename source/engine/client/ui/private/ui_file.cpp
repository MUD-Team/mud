#include "ui_file.h"

#include <RmlUi/Core/Core.h>

#include "i_system.h"
#include "physfs.h"

UIFileInterface::UIFileInterface()
{
    Rml::SetFileInterface(this);
}

UIFileInterface::~UIFileInterface()
{    
}

Rml::FileHandle UIFileInterface::Open(const Rml::String &path)
{
    PHYSFS_File *fp = PHYSFS_openRead(("client/ui/" + path).c_str());
    if (!fp)
    {
        I_Warning("RmlUi: Failed to open file %s, error code: %u", path.c_str(), PHYSFS_getLastErrorCode());
    }

    return (Rml::FileHandle)fp;
}

void UIFileInterface::Close(Rml::FileHandle file)
{
    if (!file)
    {
        I_Error("RmlUi: Requesting file close on NULL file");
    }

    if (!PHYSFS_close((PHYSFS_File *)file))
    {
        I_Warning("RmlUi: Failed to close file, error code: %u", PHYSFS_getLastErrorCode());
    }
}

size_t UIFileInterface::Read(void *buffer, size_t size, Rml::FileHandle file)
{
    if (!file)
    {
        I_Error("RmlUi: Requesting file read on NULL file");
    }

    PHYSFS_sint64 read = PHYSFS_readBytes((PHYSFS_File *)file, buffer, size);
    if (read == -1)
    {
        I_Error("RmlUi: Failed to read file, error code: %u", PHYSFS_getLastErrorCode());
    }

    return read;
}

bool UIFileInterface::Seek(Rml::FileHandle file, long offset, int origin)
{
    PHYSFS_File *pfile = (PHYSFS_File *)file;
    if (!pfile)
    {
        I_Error("RmlUi: Requesting file seek on NULL file");
    }

    if (origin != SEEK_SET && origin != SEEK_END)
    {
        I_Error("RmlUi: Requesting file seek other than SEEK_SET or SEEK_END");
    }

    if (origin == SEEK_END)
    {
        PHYSFS_sint64 length = PHYSFS_fileLength(pfile);
        if (length == -1)
        {
            I_Error("RmlUi: Cannot determine file length on SEEK_END");
        }

        return PHYSFS_seek(pfile, length + offset) != 0;
    }

    return PHYSFS_seek(pfile, offset) != 0;
}

size_t UIFileInterface::Tell(Rml::FileHandle file)
{
    if (!file)
    {
        I_Error("RmlUi: Requesting file tell on NULL file");
    }

    PHYSFS_sint64 result = PHYSFS_tell((PHYSFS_File *)file);

    if (result == -1)
    {
        I_Error("RmlUi: Failed to tell file, error code: %u", PHYSFS_getLastErrorCode());
    }

    return result;
}

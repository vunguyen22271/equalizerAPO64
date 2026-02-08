
import sys

def patch_vcxproj(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Replace Configuration|Platform
    content = content.replace("Release|Win32", "Release|x64")
    content = content.replace("Debug|Win32", "Debug|x64")
    
    # Replace Platform tag
    content = content.replace("<Platform>Win32</Platform>", "<Platform>x64</Platform>")
    
    # Replace specific Qt includes if they have win32 in path (e.g. mkspecs/win32-msvc)
    # The view_file output showed: mkspecs\win32-msvc. This might need to be unchanged if it's the valid mkspec even for x64 on some Qt versions, or maybe win32-msvc covers both?
    # Usually Qt 6 has separate mkspecs?
    # Let's check Qt mkspecs/win32-msvc.
    # Actually, often 'win32-msvc' is used for MSVC builds in general.
    # I will NOT replace win32-msvc unless I am sure.
    # The file path has msvc2022_64, so it's definitely x64 Qt.
    # So 'win32-msvc' might be correct or legacy naming.
    # I will stick to configuration replacements.

    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
    print(f"Successfully patched {filepath}")

if __name__ == "__main__":
    patch_vcxproj(sys.argv[1])

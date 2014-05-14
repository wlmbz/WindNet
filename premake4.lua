--
-- Premake4 build script (http://industriousone.com/premake/download)
--

solution 'WindNet'
    configurations {'Debug', 'Release'}    
    language 'C++'
    flags {'ExtraWarnings'}
    targetdir 'bin'
    
    configuration 'Debug'
        defines { 'DEBUG' }
        flags { 'Symbols' }
        
    configuration 'Release'
        defines { 'NDEBUG' }
        flags { 'Symbols', 'Optimize' }
        
    project 'UnitTest'
        location 'build'
        kind 'ConsoleApp'
        uuid "AB7D1C15-7A44-41a7-8864-230D8E345608"
        
        defines
        {
            "WIN32",
            "_DEBUG",
            "NOMINMAX",
            "WIN32_LEAN_AND_MEAN",
            "_WIN32_WINNT=0x0600",
            "_SCL_SECURE_NO_WARNINGS",
        }
        files 
        {
            'dep/gtest/src/gtest-all.cc',
            'test/**.h',
            'test/**.cpp',
        }
        includedirs
        {
            "src",
            "dep/gtest",
            "dep/gtest/include",
        }

        links "libNet"
        
    project 'libNet'
        location 'build'
        kind 'StaticLib'
        uuid "8701594A-72B8-4a6a-AEF3-6B41BBC33E65"
        
        defines
        {
            "WIN32",
            "_DEBUG",
            "NOMINMAX",
            "WIN32_LEAN_AND_MEAN",
            "_WIN32_WINNT=0x0600",
            "_SCL_SECURE_NO_WARNINGS",        
        }
        
        files 
        {
            'src/**.h',
            'src/**.cpp',
        }
        includedirs
        {
            "src",
        }



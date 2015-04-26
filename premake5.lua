--
-- Premake script (http://premake.github.io)
--

solution 'WindNet'
    configurations {'Debug', 'Release'}    
    language 'C++'
    platforms {'x32', 'x64'}
    --flags {'ExtraWarnings'}
    targetdir 'bin'
    
    configuration 'Debug'
        defines { 'DEBUG' }
        flags { 'Symbols' }
        
    configuration 'Release'
        defines { 'NDEBUG' }
        flags { 'Symbols', 'Optimize' }
        
    configuration 'vs*'
        defines
        {
            'WIN32',
            'WIN32_LEAN_AND_MEAN',
            '_WIN32_WINNT=0x0600',
            '_CRT_SECURE_NO_WARNINGS',
            '_SCL_SECURE_NO_WARNINGS',
            '_WINSOCK_DEPRECATED_NO_WARNINGS',
            'NOMINMAX',
        }
        links {'ws2_32', 'winmm', }
        
    project 'unittest'
        location 'build'
        kind 'ConsoleApp'
        uuid 'AB7D1C15-7A44-41a7-8864-230D8E345608'
        files 
        {
            'dep/gtest/src/gtest-all.cc',
            'test/**.h',
            'test/**.cpp',
        }
        includedirs
        {
            'src',
            'dep/gtest',
            'dep/gtest/include',
        }

        links 'windnet'
        
    project 'windnet'
        location 'build'
        kind 'StaticLib'
        files 
        {
            'src/**.h',
            'src/**.cpp',
        }
        includedirs
        {
            'src',
        }



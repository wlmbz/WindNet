--
-- Premake script (http://premake.github.io)
--

solution 'WindNet'
    configurations {'Debug', 'Release'}
    language 'C++'
    architecture 'x64'
    --flags {'ExtraWarnings'}
    targetdir 'bin'

    configuration 'Debug'
        defines { 'DEBUG' }
        flags { 'Symbols' }

    configuration 'Release'
        defines { 'NDEBUG' }
        flags { 'Symbols' }
        optimize 'On'

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

    project 'UnitTest'
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
        links
        {
            'ws2_32',
            'winmm',
            'libNet'
        }
    
    project 'Example'
        location 'build'
        kind 'ConsoleApp'
        uuid 'E693BE88-7B61-4A7F-AA01-A3FF2FDDF96F'
        files
        {
            'example/**.h',
            'example/**.cpp',
        }
        includedirs
        {
            'src'
        }
        links
        {
            'ws2_32',
            'winmm',
            'libNet'
        }
    
    project 'libNet'
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

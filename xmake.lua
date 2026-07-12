set_toolchains("gcc")
add_rules("mode.debug", "mode.release")

target("booleval")
    set_kind("binary")
    set_languages("c++26")      -- Explicitly forces Clang to use C++23
    add_files("*.cpp")
    add_files("*.ixx")          -- User defined modules
    add_cxflags("-std=c++26","-fcontracts", "-fcontract-evaluation-semantic=enforce", {force = true})  -- Explicitly forces GCC to use C++23
    add_ldflags("-lstdc++exp")
    -- Explicitly forces xmake to scan .cpp files for 'import std;'
    set_policy("build.c++.modules", true)
    
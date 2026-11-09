add_rules("mode.debug")

target("notfall_result_testing_binary")
  set_kind("binary")
  set_languages("c++20")

  add_includedirs("include")
  add_files("tests/main.cc")

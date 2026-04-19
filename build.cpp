#include <builder.h>

#include <vector>
#include <string>

static void CopyLibclang() {
#ifdef _WIN32
	system( "copy /y clang\\bin\\libclang.dll bin\\libclang.dll" );
#elif defined( __linux__ )
	system( "cp clang/lib/libclang.so bin/" );
	system( "cp clang/lib/libclang.so.20.1 bin/" );
#endif
}

BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options, CommandLineArgs* args  ) {
	bool release								= HasCommandLineArg( args, "--release" );
	options->forceRebuild 			= HasCommandLineArg( args, "--clean" );
	options->generateSolution 	= HasCommandLineArg( args, "--generate-solution" );

  options->linkAgainstWindowsDynamicRuntime = true;

#ifdef _WIN32
  std::vector<std::string> libraries = {
    "user32", "Shlwapi", "DbgHelp", "Ole32", "Advapi32", "OleAut32", "libclang"
  };
#elif defined( __linux__ )
	std::vector<std::string> libraries = { "stdc++", "uuid", "clang" };
#endif

	std::vector<std::string> ignoredWarnings = {
		"-Wno-newline-eof", "-Wno-format-nonliteral",	"-Wno-gnu-zero-variadic-macro-arguments",	"-Wno-declaration-after-statement",	"-Wno-unsafe-buffer-usage",
		"-Wno-zero-as-null-pointer-constant",	"-Wno-c++98-compat-pedantic",	"-Wno-old-style-cast","-Wno-missing-field-initializers", "-Wno-switch-default",	"-Wno-covered-switch-default",
		"-Wno-unused-function", "-Wno-unused-variable",	"-Wno-unused-but-set-variable",	"-Wno-double-promotion", "-Wno-documentation-unknown-command",
#ifdef _WIN32
		"-Wno-switch",
#elif defined( __linux__ )
	  "-Wno-cast-align", "-Wno-alloca",	"-Wno-padded",
#endif
	};

	// Shared base of config similar to the build scripts common file
	BuildConfig baseConfig = {
		.binaryFolder               	= "bin",
		.sourceFiles                	= { "src/builder.cpp", "src/visual_studio.cpp", "src/core/src/core.suc.cpp", "src/backend_clang.cpp", "src/backend_msvc.cpp", "src/win_support.cpp", "src/vs_code.cpp" },
		.defines                    	= {"_CRT_SECURE_NO_WARNINGS", "CORE_USE_XXHASH", "CORE_USE_SUBPROCESS", "CORE_SUC", "HASHMAP_HIDE_MISSING_KEY_WARNING", "HLML_NAMESPACE"},
		.additionalIncludes         	= {"src/core/include", "clang/include"},
		.additionalLibPaths         	= { "clang/lib" },
		.additionalLibs             	= std::move(libraries),
		.warningLevels              	= { "-Wall", "-Wextra", "-Weverything", "-Wpedantic" },
		.ignoreWarnings             	= std::move(ignoredWarnings),
		.additionalCompilerArguments	= { "-ferror-limit=0" },
		.languageVersion            	= LANGUAGE_VERSION_CPP20,
		.optimizationLevel          	= release ? OPTIMIZATION_LEVEL_O3 : OPTIMIZATION_LEVEL_O0,
		.removeSymbols              	= release,
		.warningsAsErrors           	= true,
		.OnPostBuild                	= CopyLibclang,
	};

#ifdef __linux__
	baseConfig.additionalCompilerArguments.push_back( "-Wl -rpath=bin" );
#endif
  
  if( release ) {
    baseConfig.defines.push_back( "NDEBUG" );
    baseConfig.defines.push_back( "BUILDER_PROGRAM_NAME=\"builder\"" );
  } else {
    baseConfig.defines.push_back( "_DEBUG" );
    baseConfig.defines.push_back( "BUILDER_PROGRAM_NAME=\"builder_debug\"" );
  }

	// Config for building Builder itself.
	BuildConfig builderConfig = baseConfig;
	builderConfig.name					= "builder";
	builderConfig.binaryFolder  = "bin";
	builderConfig.binaryName    = release ? "builder" : "builder_debug";
	builderConfig.sourceFiles.push_back( "src/main.cpp" );
#ifdef _WIN32
	builderConfig.ignoreWarnings.push_back( "-Wno-cast-align" );
	builderConfig.ignoreWarnings.push_back( "-Wno-nontrivial-memcall" );
#endif

	AddBuildConfig( options, &builderConfig );

	// Config for building the tests for Builder.
	BuildConfig builderTestsConfig = baseConfig;
	builderTestsConfig.name 					= "tests";
	builderTestsConfig.binaryName 		= release ? "builder_tests_release" : "builder_tests_debug";
	builderTestsConfig.removeSymbols	= false;
	builderTestsConfig.sourceFiles.push_back( "tests/tests_main.cpp" );
	if ( release ) {
		builderTestsConfig.additionalCompilerArguments.push_back( "-fexceptions" );
		builderTestsConfig.additionalCompilerArguments.push_back( "-ffast-math" );
	}

	AddBuildConfig( options, &builderTestsConfig );

	if ( options->generateSolution ) {
		options->solution = {
			.name = "builder.sln",
			.platforms = { "x64" },
			.projects = { {
					.name = "builder",
					.configs = {
						{ "Debug",   builderConfig, {             }, {} },
						{ "Release", builderConfig, { "--release" }, {} },
					},
				}, {
					.name = "tests",
					.configs = {
						{ "Debug",   builderTestsConfig, {             }, {} },
						{ "Release", builderTestsConfig, { "--release" }, {} },
					},
				}
			},
		};
	}
}

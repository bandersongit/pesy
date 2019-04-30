module Mode = Mode;
module PesyConf = PesyConf;
module EsyCommand = EsyCommand;

open Utils;
open NoLwt;
open Printf;
open EsyCommand;

let bootstrapIfNecessary = projectPath => {
  let packageNameKebab = kebab(Filename.basename(projectPath));
  let packageNameKebabSansScope = removeScope(packageNameKebab);
  let packageNameUpperCamelCase = upperCamelCasify(packageNameKebabSansScope);
  let version = "0.0.0";
  let packageJSONTemplate = loadTemplate("pesy-package.template.json");
  let appReTemplate = loadTemplate("pesy-App.template.re");
  let testReTemplate = loadTemplate("pesy-Test.template.re");
  let runTestsTemplate = loadTemplate("pesy-RunTests.template.re");
  let testFrameworkTemplate = loadTemplate("pesy-TestFramework.template.re");
  let utilRe = loadTemplate("pesy-Util.template.re");
  let readMeTemplate = loadTemplate("pesy-README.template.md");
  let gitignoreTemplate = loadTemplate("pesy-gitignore.template");
  let esyBuildStepsTemplate =
    loadTemplate(
      Path.("azure-pipeline-templates" / "pesy-esy-build-steps.template.yml"),
    );
  let packageLibName = packageNameKebabSansScope ++ "/library";
  let packageTestName = packageNameKebabSansScope ++ "/test";

  let substituteTemplateValues = s =>
    s
    |> Str.global_replace(r("<PACKAGE_NAME_FULL>"), packageNameKebab)
    |> Str.global_replace(r("<VERSION>"), version)
    |> Str.global_replace(r("<PUBLIC_LIB_NAME>"), packageLibName)
    |> Str.global_replace(r("<TEST_LIB_NAME>"), packageTestName)
    |> Str.global_replace(r("<PACKAGE_NAME>"), packageNameKebab)
    |> Str.global_replace(
         r("<PACKAGE_NAME_UPPER_CAMEL>"),
         packageNameUpperCamelCase,
       );

  if (!exists("package.json")) {
    let packageJSON = packageJSONTemplate |> substituteTemplateValues;
    write("package.json", packageJSON);
  };

  let appReDir = Path.(projectPath / "executable");
  let appRePath = Path.(appReDir / packageNameUpperCamelCase ++ "App.re");

  if (!exists(appRePath)) {
    let appRe = appReTemplate |> substituteTemplateValues;
    let _ = mkdirp(appReDir);
    write(appRePath, appRe);
  };

  let utilReDir = Path.(projectPath / "library");
  let utilRePath = Path.(utilReDir / "Util.re");

  if (!exists(utilRePath)) {
    mkdirp(utilReDir);
    write(utilRePath, utilRe);
  };

  let testReDir = Path.(projectPath / "test");

  if (!exists(testReDir)) {
    let _ = mkdirp(testReDir);
    ();
  };

  let testRePath =
    Path.(testReDir / "Test" ++ packageNameUpperCamelCase ++ ".re");

  if (!exists(testRePath)) {
    let testRe = testReTemplate |> substituteTemplateValues;
    write(testRePath, testRe);
  };

  let testFrameworkPath = Path.(testReDir / "TestFramework.re");
  if (!exists(testFrameworkPath)) {
    let testFramework = testFrameworkTemplate |> substituteTemplateValues;
    write(testFrameworkPath, testFramework);
  };

  let testExeDir = Path.(projectPath / "testExe");
  if (!exists(testExeDir)) {
    let _ = mkdirp(testExeDir);
    ();
  };

  let testExeFileName =
    "Run<PACKAGE_NAME_UPPER_CAMEL>Tests.re" |> substituteTemplateValues;
  let runTestsPath = Path.(testExeDir / testExeFileName);
  if (!exists(runTestsPath)) {
    let runTests = runTestsTemplate |> substituteTemplateValues;

    write(runTestsPath, runTests);
  };

  let readMePath = Path.(projectPath / "README.md");

  if (!exists(readMePath)) {
    let readMe = readMeTemplate |> substituteTemplateValues;

    write(readMePath, readMe);
  };

  let gitignorePath = Path.(projectPath / ".gitignore");

  if (!exists(gitignorePath)) {
    let gitignore = gitignoreTemplate |> substituteTemplateValues;

    write(".gitignore", gitignore);
  };

  let azurePipelinesPath = Path.(projectPath / "azure-pipelines.yml");

  if (!exists(azurePipelinesPath)) {
    let esyBuildSteps = esyBuildStepsTemplate |> substituteTemplateValues;

    copyTemplate(
      Path.("azure-pipeline-templates" / "pesy-azure-pipelines.yml"),
      Path.(projectPath / "azure-pipelines.yml"),
    );
    mkdirp(".ci");
    let ciFilesPath = Path.(projectPath / ".ci");
    write(Path.(ciFilesPath / "esy-build-steps.yml"), esyBuildSteps);
    copyTemplate(
      Path.("azure-pipeline-templates" / "pesy-publish-build-cache.yml"),
      Path.(ciFilesPath / "publish-build-cache.yml"),
    );
    copyTemplate(
      Path.("azure-pipeline-templates" / "pesy-restore-build-cache.yml"),
      Path.(ciFilesPath / "restore-build-cache.yml"),
    );
  };

  let libKebab = packageNameKebabSansScope;
  let duneProjectFile = Path.(projectPath / "dune-project");
  if (!exists(duneProjectFile)) {
    write(duneProjectFile, spf({|(lang dune 1.2)
(name %s)
|}, libKebab));
  };

  let opamFileName = libKebab ++ ".opam";
  let opamFile = Path.(projectPath / opamFileName);
  if (!exists(opamFile)) {
    write(opamFile, "");
  };

  let rootDuneFile = Path.(projectPath / "dune");

  if (!exists(rootDuneFile)) {
    write(rootDuneFile, "(ignored_subdirs (node_modules))");
  };
  (
    packageNameKebabSansScope,
    version,
    packageNameUpperCamelCase,
    packageLibName,
  );
};

let generateBuildFiles = projectRoot => {
  let packageJSONPath = Path.(projectRoot / "package.json");
  PesyConf.gen(projectRoot, packageJSONPath);
};

let build = projectRoot => {
  let packageJSONPath = Path.(projectRoot / "package.json");
  /* Will throw if there validation errors. We catch then at Pesy.re
   * Else, returns the build target name
   */
  PesyConf.validateDuneFiles(projectRoot, packageJSONPath);
};

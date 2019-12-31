from conans import ConanFile, CMake, tools


class ApophenicConan(ConanFile):
	name = "apophenic"
	version = "master"
	license = "WTFPL2"
	author = "konrad.no.tantoo"
	url = "https://github.com/KonradNoTantoo/apophenic"
	description = "Apophenia is the tendency to mistakenly perceive connections and meaning between unrelated things."
	topics = ("patterns", "C++", "conan")
	settings = "os", "compiler", "build_type", "arch"
	options = {"tests": [True, False]}
	default_options = {"tests": False}
	generators = "cmake"
	no_copy_source = True
	folder_name = "{}-{}".format(name, version)
	scm = {
		"type": "git",  # Use "type": "svn", if local repo is managed using SVN
		"url": "auto",
		"revision": "auto"
	}
	def build_requirements(self):
		if self.options.tests:
			self.build_requires("gtest/1.8.1")

	def source(self):
		git = tools.Git(folder=self.folder_name)
		git.clone("https://github.com/KonradNoTantoo/apophenic", self.version)

		if self.options.tests:
			tools.replace_in_file("{}/CMakeLists.txt".format(self.folder_name),
								  "project (apophenic)",
								  '''project (apophenic)
include (${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup ()''')
			tools.replace_in_file("{}/CMakeLists.txt".format(self.folder_name),
								  " pthread", "")

	def build(self): # this is not building a library, just tests
		if self.options.tests:
			cmake = CMake(self)
			cmake.definitions["BUILD_TESTS"] = "ON"
			cmake.configure(source_folder=self.folder_name)
			cmake.build()
			cmake.test()

	def package(self):
		self.copy("*.hxx")

	def package_id(self):
		self.info.header_only()


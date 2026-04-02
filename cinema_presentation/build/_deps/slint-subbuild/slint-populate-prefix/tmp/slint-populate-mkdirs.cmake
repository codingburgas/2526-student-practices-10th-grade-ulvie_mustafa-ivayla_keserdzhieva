# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "D:/schoolProjects/sprints/2526-student-practices-10th-grade-ulvie_mustafa-ivayla_keserdzhieva/cinema_presentation/build/_deps/slint-src")
  file(MAKE_DIRECTORY "D:/schoolProjects/sprints/2526-student-practices-10th-grade-ulvie_mustafa-ivayla_keserdzhieva/cinema_presentation/build/_deps/slint-src")
endif()
file(MAKE_DIRECTORY
  "D:/schoolProjects/sprints/2526-student-practices-10th-grade-ulvie_mustafa-ivayla_keserdzhieva/cinema_presentation/build/_deps/slint-build"
  "D:/schoolProjects/sprints/2526-student-practices-10th-grade-ulvie_mustafa-ivayla_keserdzhieva/cinema_presentation/build/_deps/slint-subbuild/slint-populate-prefix"
  "D:/schoolProjects/sprints/2526-student-practices-10th-grade-ulvie_mustafa-ivayla_keserdzhieva/cinema_presentation/build/_deps/slint-subbuild/slint-populate-prefix/tmp"
  "D:/schoolProjects/sprints/2526-student-practices-10th-grade-ulvie_mustafa-ivayla_keserdzhieva/cinema_presentation/build/_deps/slint-subbuild/slint-populate-prefix/src/slint-populate-stamp"
  "D:/schoolProjects/sprints/2526-student-practices-10th-grade-ulvie_mustafa-ivayla_keserdzhieva/cinema_presentation/build/_deps/slint-subbuild/slint-populate-prefix/src"
  "D:/schoolProjects/sprints/2526-student-practices-10th-grade-ulvie_mustafa-ivayla_keserdzhieva/cinema_presentation/build/_deps/slint-subbuild/slint-populate-prefix/src/slint-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/schoolProjects/sprints/2526-student-practices-10th-grade-ulvie_mustafa-ivayla_keserdzhieva/cinema_presentation/build/_deps/slint-subbuild/slint-populate-prefix/src/slint-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/schoolProjects/sprints/2526-student-practices-10th-grade-ulvie_mustafa-ivayla_keserdzhieva/cinema_presentation/build/_deps/slint-subbuild/slint-populate-prefix/src/slint-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()

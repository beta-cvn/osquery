/**
 * Copyright (c) 2014-present, The osquery authors
 *
 * This source code is licensed as defined by the LICENSE file found in the
 * root directory of this source tree.
 *
 * SPDX-License-Identifier: (Apache-2.0 OR GPL-2.0-only)
 */

#pragma once

#include <set>
#include <string>

#include <osquery/dispatcher/dispatcher.h>
#include <osquery/filesystem/filesystem.h>
#include <osquery/utils/status/status.h>

namespace osquery {

/// Database domain where we store carve table entries
const std::string kCarveDbDomain = "carves";

/// Prefix used for the temp FS where carved files are stored
const std::string kCarvePathPrefix = "osquery_carve_";

/// Prefix applied to the file carve tar archive.
const std::string kCarveNamePrefix = "carve_";

/// Database prefix used to directly access and manipulate our carver entries
const std::string kCarverDBPrefix = "carves.";

class Carver : public InternalRunnable {
 public:
  Carver(const std::set<std::string>& paths,
         const std::string& guid,
         const std::string& requestId);

  ~Carver();

  /*
   * @brief A helper function to perform a start to finish carve
   *
   * This function walks through the carve, compress, and exfil functions
   * in one fell swoop. Use of this class should largely happen through
   * this function.
   */
  void start() override;

 protected:
  /*
   * @brief A helper function that 'carves' all files from disk
   *
   * This function copies all source files to a temporary directory and returns
   * a list of all destination files.
   */
  std::set<boost::filesystem::path> carveAll();

  /*
   * @brief A helper function that does a blockwise copy from src to dst
   *
   * This function copies the source file to the destination file, doing so
   * by blocks specified with FLAGS_carver_block_size (defaults to 8K)
   */
  Status blockwiseCopy(PlatformFile& src, PlatformFile& dst);

  /*
   * @brief Helper function to POST a carve to the graph endpoint.
   *
   * Once all of the files have been carved and the tgz has been
   * created, we POST the carved file to an endpoint specified by the
   * carver_start_endpoint and carver_continue_endpoint
   */
  virtual Status postCarve(const boost::filesystem::path& path);

  // Getter for the carver status
  Status getStatus() {
    return status_;
  }

  // Helper function to return the carve directory
  boost::filesystem::path getCarveDir() {
    return carveDir_;
  }

 private:
  /*
   * @brief a variable to keep track of the temp fs used in carving
   *
   * This variable represents the location in which we store all of our carved
   * files. When a carve has completed all of the desired files, as well
   * as the tar archive should reside in this directory
   */
  boost::filesystem::path carveDir_;

  /*
   * @brief a variable tracking all of the paths we attempt to carve
   *
   * This is a globbed set of file paths that we're expecting will be
   * carved.
   */
  std::set<boost::filesystem::path> carvePaths_;

  /*
   * @brief a helper variable for keeping track of the posix tar archive.
   *
   * This variable is the absolute location of the tar archive created from
   * tar'ing all of the carved files from the carve temp dir.
   */
  boost::filesystem::path archivePath_;

  /*
   * @brief a helper variable for keeping track of the compressed tar.
   *
   * This variable is the absolute location of the tar archive created from
   * zstd of the archive.
   */
  boost::filesystem::path compressPath_;

  /*
   * @brief a unique ID identifying the 'carve'
   *
   * This unique generated GUID is used to identify the carve session from
   * other carves. It is also used by our backend service to derive a
   * session key for exfiltration.
   */
  std::string carveGuid_;

  /**
   * @brief the distributed work ID of a carve
   *
   * This value should be used by the TLS endpoints where carve data is
   * aggregated, to tie together a distributed query with the carve data
   */
  std::string requestId_;

  // Running status of the carver
  Status status_;
};

/**
 * @brief Start a file carve of the given paths
 *
 * @return A status returning if the carves were started successfully
 */
Status carvePaths(const std::set<std::string>& paths);
} // namespace osquery

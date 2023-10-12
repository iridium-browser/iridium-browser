// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "third_party/protobuf/src/google/protobuf/compiler/importer.h"
#include "third_party/protobuf/src/google/protobuf/descriptor.h"

namespace {

using ::google::protobuf::Descriptor;
using ::google::protobuf::FileDescriptor;
using ::google::protobuf::MethodDescriptor;
using ::google::protobuf::ServiceDescriptor;
using ::google::protobuf::compiler::DiskSourceTree;
using ::google::protobuf::compiler::Importer;
using ::google::protobuf::compiler::MultiFileErrorCollector;

// Command line flag for the path to proto file to parse.
constexpr const char kProtoFlag[] = "--proto";

// Command line flag to for additional includes to be added to the generated
// file.
constexpr const char kIncludesFlag[] = "--include";

// Command line flag for target output directory for the generated file.
constexpr const char kOutDirFlag[] = "--out-dir";

// Command line flag for source root directory for the generated file.
constexpr const char kSourceDirFlag[] = "--source-dir";

// Command line flag for additional import directories for proto lookup.
constexpr const char kImportDir[] = "--import-dir";

// Suffix added to the generated file.
constexpr const char kCastCorePbExtension[] = ".castcore.pb.h";

// Extension of gRPC protobuf file.
constexpr const char kGrpcPbExtension[] = ".grpc.pb.h";

static const bool is_debug = false;

std::string RemoveProtoExtension(const std::string& file_path) {
  static const std::string kProtoExtension = ".proto";
  if (file_path.length() < kProtoExtension.length()) {
    std::cerr << "[ERROR] File path is too short: " << file_path << std::endl;
    exit(1);
  }

  int ext_pos = file_path.length() - kProtoExtension.length();
  if (file_path.substr(ext_pos) != kProtoExtension) {
    std::cerr << "[ERROR] Not a proto file: " << file_path << std::endl;
    exit(1);
  }

  return file_path.substr(0, ext_pos);
}

std::string ReplaceString(const std::string& input,
                          const std::string& old_token,
                          const std::string& new_token) {
  std::string output = input;
  std::string::size_type pos;
  while ((pos = output.find(old_token)) != std::string::npos) {
    output.replace(pos, old_token.length(), new_token);
  }
  return output;
}

class SimpleErrorCollector : public MultiFileErrorCollector {
 public:
  void AddError(const std::string& filename, int line, int column,
                   const std::string& message) override {
    std::cerr << "[ERROR] " << filename << ", Line " << line << ", Column "
              << column << ": " << message << std::endl;
  }
};

class Includes {
 public:
  explicit Includes(std::vector<std::string> includes)
      : includes_(includes.begin(), includes.end()) {}

  void AddInclude(std::string include_file_path) {
    includes_.insert(include_file_path);
  }

  void PrintIncludePrologues(std::ostream& os) const {
    for (const auto& include : includes_) {
      os << "#include \"" << include << "\"" << std::endl;
    }
    os << std::endl;
  }

 private:
  std::set<std::string> includes_;
};

class SourceProto {
 public:
  SourceProto(std::string proto_file_path, Includes* includes)
      : proto_file_path_(std::move(proto_file_path)),
        output_file_path_(RemoveProtoExtension(proto_file_path_)),
        cpp_headerguard_(CreateHeaderGuard(output_file_path_)),
        includes_(includes) {}

  bool Initialize(std::string source_dir,
                  std::vector<std::string> import_dirs) {
    includes_->AddInclude(output_file_path_ + kGrpcPbExtension);

    source_tree_.MapPath("", source_dir);
    for (const auto& import_dir : import_dirs) {
      source_tree_.MapPath("", source_dir + import_dir);
    }
    importer_ = std::make_unique<Importer>(&source_tree_, &error_collector_);

    file_descriptor_ = importer_->Import(proto_file_path_);
    if (!file_descriptor_) {
      return false;
    }

    if (file_descriptor_->service_count() == 0) {
      std::cout << "[WARNING] At least one service should be specified in the "
                   "proto file: "
                << proto_file_path_ << std::endl;
    }

    const std::string& package = file_descriptor_->package();
    std::string::size_type start_pos = 0;
    std::string::size_type next_pos;
    while ((next_pos = package.find(".", start_pos)) != std::string::npos) {
      namespaces_.push_back(package.substr(start_pos, next_pos - start_pos));
      start_pos = next_pos + 1;
    }
    namespaces_.push_back(package.substr(start_pos));
    return true;
  }

  bool Generate(std::string out_dir) const {
    std::ostringstream header;

    PrintHeaderGruardPrologues(header);
    includes_->PrintIncludePrologues(header);
    PrintNamespacePrologues(header);

    for (int i = 0; i < file_descriptor_->service_count(); ++i) {
      if (is_debug) {
        std::cout << "[INFO] Generating Cast Core gRPC definitions for "
                  << file_descriptor_->service(i)->name() << std::endl;
      }
      PrintCastCoreHandlerDefinition(header, file_descriptor_->service(i));
      PrintCastCoreStubDefinition(header, file_descriptor_->service(i));
    }

    PrintNamespaceEpilogues(header);
    PrintHeaderGruardEpilogues(header);

    auto target_file = out_dir + output_file_path_ + kCastCorePbExtension;
    if (is_debug) {
      std::cout << "[INFO] Writing generated files: " << target_file
                << std::endl;
    }

    std::ofstream cpp_headerfile(target_file);
    if (!cpp_headerfile) {
      std::cerr << "[ERROR] Failed to open target header file: " << target_file
                << std::endl;
      return false;
    }

    cpp_headerfile << header.str();
    return true;
  }

 private:
  void PrintHeaderGruardPrologues(std::ostream& os) const {
    os << "#ifndef " << cpp_headerguard_ << std::endl
       << "#define " << cpp_headerguard_ << std::endl
       << std::endl;
  }

  void PrintHeaderGruardEpilogues(std::ostream& os) const {
    os << "#endif  // " << cpp_headerguard_ << std::endl;
  }

  void PrintNamespacePrologues(std::ostream& os) const {
    for (const auto& ns : namespaces_) {
      os << "namespace " << ns << " {" << std::endl;
    }
    os << std::endl;
  }

  void PrintNamespaceEpilogues(std::ostream& os) const {
    for (const auto& ns : namespaces_) {
      os << "}  // namespace " << ns << std::endl;
    }
    os << std::endl;
  }

  void PrintCastCoreHandlerDefinition(
      std::ostream& header, const ServiceDescriptor* service_descriptor) const {
    const std::string service_name = service_descriptor->name();
    std::ostringstream method_names;
    std::ostringstream class_methods;
    for (int i = 0; i < service_descriptor->method_count(); ++i) {
      const MethodDescriptor* method = service_descriptor->method(i);
      if (method->client_streaming()) {
        std::cout << "[WARNING] Client streaming APIs are not supported yet - "
                     "skipping "
                  << method->name() << std::endl;
        continue;
      }

      const Descriptor* input = method->input_type();
      const Descriptor* output = method->output_type();
      const std::string method_name_var =
          "k" + service_name + "_" + method->name() + "_MethodName";
      method_names << "constexpr char " << method_name_var << "[] = \""
                   << method->name() << "\";" << std::endl;
      class_methods << "    using " << method->name() << " = ";
      if (method->server_streaming()) {
        class_methods << "::cast::utils::GrpcServerStreamingHandler<";
      } else {
        class_methods << "::cast::utils::GrpcUnaryHandler<";
      }
      const auto request_type = FullTypeName(input->full_name());
      const auto response_type = FullTypeName(output->full_name());
      class_methods << service_name << ", " << request_type << ", "
                    << response_type << ", " << method_name_var << ">;"
                    << std::endl;
    }
    header << "// " << service_name << " gRPC handler." << std::endl
           << method_names.str() << std::endl
           << "class " << service_name << "Handler {" << std::endl
           << "  public: " << std::endl
           << class_methods.str() << "};" << std::endl
           << std::endl;
  }

  void PrintCastCoreStubDefinition(
      std::ostream& header, const ServiceDescriptor* service_descriptor) const {
    const std::string service_name = service_descriptor->name();
    header << "// " << service_name << " gRPC stub." << std::endl
           << "class " << service_name << "Stub : "
           << " public ::cast::utils::GrpcStub<" << service_name << "> {"
           << std::endl
           << " public:" << std::endl
           << "  using GrpcStub::GrpcStub;" << std::endl
           << "  using GrpcStub::operator=;" << std::endl
           << "  using GrpcStub::AsyncInterface;" << std::endl
           << "  using GrpcStub::CreateCall;" << std::endl
           << "  using GrpcStub::SyncInterface;" << std::endl
           << std::endl;
    for (int i = 0; i < service_descriptor->method_count(); ++i) {
      const MethodDescriptor* method = service_descriptor->method(i);
      if (method->client_streaming()) {
        std::cout << "[WARNING] Client streaming APIs are not supported yet - "
                     "skipping "
                  << method->name() << std::endl;
        continue;
      }

      const Descriptor* input = method->input_type();
      const Descriptor* output = method->output_type();
      const std::string method_name_var = "k" + method->name() + "Method";
      header << "  using " << method->name() << " = ";
      if (method->server_streaming()) {
        header << "::cast::utils::GrpcServerStreamingCall<";
      } else {
        header << "::cast::utils::GrpcUnaryCall<";
      }
      const auto request_type = FullTypeName(input->full_name());
      const auto response_type = FullTypeName(output->full_name());
      header << service_name << "Stub, " << request_type << ", "
             << response_type << ", &AsyncInterface::" << method->name();
      if (!method->client_streaming() && !method->server_streaming()) {
        // Add the sync interface defs
        header << ", &SyncInterface::" << method->name();
      }
      header << ">;" << std::endl;
    }
    header << "};" << std::endl << std::endl;
  }

  static std::string FullTypeName(const std::string& proto_type_name) {
    return "::" + ReplaceString(proto_type_name, ".", "::");
  }

  static std::string CreateHeaderGuard(const std::string& proto_file_path) {
    std::ostringstream cpp_headerguard;
    std::string result = proto_file_path;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) -> unsigned char {
                     if (c == '.' || c == '/' || c == '\\') {
                       return '_';
                     }
                     return std::toupper(c);
                   });
    result.append("_CASTCORE_PB_H_");
    return result;
  }

  const std::string proto_file_path_;
  const std::string output_file_path_;
  const std::string cpp_headerguard_;
  Includes* const includes_;

  SimpleErrorCollector error_collector_;
  DiskSourceTree source_tree_;
  std::unique_ptr<Importer> importer_;
  const FileDescriptor* file_descriptor_;

  std::vector<std::string> namespaces_;
};

}  // namespace

int main(int argc, char** argv) {
  std::string flag;
  std::string proto_file_path;
  std::string out_dir;
  std::string source_dir;
  std::vector<std::string> include_file_paths;
  std::vector<std::string> import_dirs;
  for (int i = 1; i < argc; ++i) {
    if (flag.empty()) {
      flag = argv[i];
      continue;
    }
    if (flag == kProtoFlag) {
      proto_file_path = argv[i];
    } else if (flag == kIncludesFlag) {
      include_file_paths.push_back(argv[i]);
    } else if (flag == kImportDir) {
      import_dirs.push_back(argv[i]);
    } else if (flag == kOutDirFlag) {
      out_dir = argv[i];
      if (out_dir.empty()) {
        std::cerr << "[ERROR] Output directory must be specified" << std::endl;
        exit(1);
      }
      if (out_dir[out_dir.length() - 1] != '/') {
        out_dir.append("/");
      }
    } else if (flag == kSourceDirFlag) {
      source_dir = argv[i];
    } else {
      std::cerr << "[ERROR] Unexpected flag: " << flag << std::endl;
      exit(1);
    }
    flag.clear();
  }

  if (proto_file_path.empty()) {
    std::cerr << "[ERROR] Proto file cannot be empty" << std::endl;
    return -1;
  }

  if (is_debug) {
    std::cout << "[INFO] Generating Cast Core gRPC definitions: proto="
              << proto_file_path << ", out_dir=" << out_dir
              << ", source_dir=" << source_dir << std::endl;
  }

  Includes includes(std::move(include_file_paths));
  SourceProto source_proto(std::move(proto_file_path), &includes);
  if (!source_proto.Initialize(std::move(source_dir), std::move(import_dirs))) {
    return -2;
  }
  if (!source_proto.Generate(std::move(out_dir))) {
    return -3;
  }
  return 0;
}

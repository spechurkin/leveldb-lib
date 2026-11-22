#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <leveldb/env.h>
#ifdef DeleteFile
#undef DeleteFile
#endif
#include <algorithm>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace {
std::wstring wide(const std::string& s) {
  int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.c_str(),-1,nullptr,0);
  if (!n) return {};
  std::wstring w(static_cast<size_t>(n),L'\0');
  MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.c_str(),-1,&w[0],n);
  w.pop_back(); return w;
}
std::string utf8(const wchar_t* w) {
  int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,w,-1,nullptr,0,nullptr,nullptr);
  if (!n) return {};
  std::string s(static_cast<size_t>(n),'\0');
  WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,w,-1,&s[0],n,nullptr,nullptr);
  s.pop_back(); return s;
}
leveldb::Status error(const std::string& path, DWORD code=GetLastError(), bool locking=false) {
  wchar_t* message=nullptr;
  FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_IGNORE_INSERTS,
    nullptr,code,0,reinterpret_cast<LPWSTR>(&message),0,nullptr);
  std::string m=message ? utf8(message) : "Windows error "+std::to_string(code);
  if (message) LocalFree(message);
  if (code==ERROR_FILE_NOT_FOUND || code==ERROR_PATH_NOT_FOUND) return leveldb::Status::NotFound(path,m);
  if (locking && (code==ERROR_LOCK_VIOLATION || code==ERROR_SHARING_VIOLATION))
    return leveldb::Status::IOError("multileveldb:database_locked "+path,m);
  return leveldb::Status::IOError(path,m);
}
struct Handle {
  HANDLE h;
  explicit Handle(HANDLE value):h(value) {}
  ~Handle() { if (h!=INVALID_HANDLE_VALUE) CloseHandle(h); }
  Handle(const Handle&)=delete;
  Handle& operator=(const Handle&)=delete;
};
leveldb::Status read(HANDLE h,size_t n,leveldb::Slice* result,char* scratch,const std::string& path) {
  size_t total=0;
  while (total<n) {
    DWORD got=0, want=static_cast<DWORD>(std::min<size_t>(n-total,MAXDWORD));
    if (!ReadFile(h,scratch+total,want,&got,nullptr)) return error(path);
    total+=got; if (got<want) break;
  }
  *result=leveldb::Slice(scratch,total); return leveldb::Status::OK();
}
class Sequential final:public leveldb::SequentialFile {
  Handle file; std::string path;
 public:
  Sequential(HANDLE h,std::string p):file(h),path(std::move(p)) {}
  leveldb::Status Read(size_t n,leveldb::Slice* s,char* b) override { return read(file.h,n,s,b,path); }
  leveldb::Status Skip(uint64_t n) override {
    LARGE_INTEGER distance; distance.QuadPart=static_cast<LONGLONG>(n);
    return SetFilePointerEx(file.h,distance,nullptr,FILE_CURRENT)?leveldb::Status::OK():error(path);
  }
};
class Random final:public leveldb::RandomAccessFile {
  Handle file; std::string path; mutable std::mutex mutex;
 public:
  Random(HANDLE h,std::string p):file(h),path(std::move(p)) {}
  leveldb::Status Read(uint64_t offset,size_t n,leveldb::Slice* s,char* b) const override {
    std::lock_guard<std::mutex> lock(mutex);
    LARGE_INTEGER position; position.QuadPart=static_cast<LONGLONG>(offset);
    if (!SetFilePointerEx(file.h,position,nullptr,FILE_BEGIN)) return error(path);
    return read(file.h,n,s,b,path);
  }
};
class Writable final:public leveldb::WritableFile {
  Handle file; std::string path;
 public:
  Writable(HANDLE h,std::string p):file(h),path(std::move(p)) {}
  leveldb::Status Append(const leveldb::Slice& s) override {
    size_t total=0;
    while(total<s.size()) {
      DWORD wrote=0, want=static_cast<DWORD>(std::min<size_t>(s.size()-total,MAXDWORD));
      if (!WriteFile(file.h,s.data()+total,want,&wrote,nullptr)) return error(path);
      if (!wrote) return leveldb::Status::IOError(path,"WriteFile made no progress");
      total+=wrote;
    }
    return leveldb::Status::OK();
  }
  leveldb::Status Close() override {
    if(file.h==INVALID_HANDLE_VALUE) return leveldb::Status::OK();
    HANDLE h=file.h; file.h=INVALID_HANDLE_VALUE;
    return CloseHandle(h)?leveldb::Status::OK():error(path);
  }
  leveldb::Status Flush() override { return leveldb::Status::OK(); }
  leveldb::Status Sync() override { return FlushFileBuffers(file.h)?leveldb::Status::OK():error(path); }
};
struct Lock final:leveldb::FileLock { Handle file; explicit Lock(HANDLE h):file(h) {} };
class Logger final:public leveldb::Logger {
  FILE* file; std::mutex mutex;
 public:
  explicit Logger(FILE* f):file(f) {}
  ~Logger() override { std::fclose(file); }
  void Logv(const char* fmt,va_list args) override {
    std::lock_guard<std::mutex> lock(mutex);
    std::vfprintf(file,fmt,args); std::fputc('\n',file); std::fflush(file);
  }
};
class Utf8Env final:public leveldb::EnvWrapper {
 public:
  Utf8Env():EnvWrapper(leveldb::Env::Default()) {}
  leveldb::Status NewSequentialFile(const std::string& p,leveldb::SequentialFile** out) override {
    *out=nullptr; HANDLE h=CreateFileW(wide(p).c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
      nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h==INVALID_HANDLE_VALUE) return error(p);
    try { *out=new Sequential(h,p); } catch(...) { CloseHandle(h); throw; }
    return leveldb::Status::OK();
  }
  leveldb::Status NewRandomAccessFile(const std::string& p,leveldb::RandomAccessFile** out) override {
    *out=nullptr; HANDLE h=CreateFileW(wide(p).c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
      nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h==INVALID_HANDLE_VALUE) return error(p);
    try { *out=new Random(h,p); } catch(...) { CloseHandle(h); throw; }
    return leveldb::Status::OK();
  }
  leveldb::Status writable(const std::string& p,leveldb::WritableFile** out,bool append) {
    *out=nullptr; HANDLE h=CreateFileW(wide(p).c_str(),GENERIC_WRITE,FILE_SHARE_READ,
      nullptr,append?OPEN_ALWAYS:CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h==INVALID_HANDLE_VALUE) return error(p);
    if(append) {
      LARGE_INTEGER zero; zero.QuadPart=0;
      if(!SetFilePointerEx(h,zero,nullptr,FILE_END)) { auto s=error(p); CloseHandle(h); return s; }
    }
    try { *out=new Writable(h,p); } catch(...) { CloseHandle(h); throw; }
    return leveldb::Status::OK();
  }
  leveldb::Status NewWritableFile(const std::string& p,leveldb::WritableFile** out) override { return writable(p,out,false); }
  leveldb::Status NewAppendableFile(const std::string& p,leveldb::WritableFile** out) override { return writable(p,out,true); }
  bool FileExists(const std::string& p) override { return GetFileAttributesW(wide(p).c_str())!=INVALID_FILE_ATTRIBUTES; }
  leveldb::Status GetChildren(const std::string& p,std::vector<std::string>* out) override {
    out->clear(); WIN32_FIND_DATAW data;
    HANDLE h=FindFirstFileW(wide(p+"/*").c_str(),&data);
    if(h==INVALID_HANDLE_VALUE) return error(p);
    try {
      do { auto name=utf8(data.cFileName); if(name!="." && name!="..") out->push_back(name); } while(FindNextFileW(h,&data));
    } catch(...) { FindClose(h); throw; }
    DWORD code=GetLastError(); FindClose(h);
    return code==ERROR_NO_MORE_FILES ? leveldb::Status::OK():error(p,code);
  }
  leveldb::Status RemoveFile(const std::string& p) override { return DeleteFileW(wide(p).c_str())?leveldb::Status::OK():error(p); }
  leveldb::Status DeleteFile(const std::string& p) override { return RemoveFile(p); }
  leveldb::Status CreateDir(const std::string& p) override {
    if(CreateDirectoryW(wide(p).c_str(),nullptr)) return leveldb::Status::OK();
    DWORD code=GetLastError();
    if(code==ERROR_ALREADY_EXISTS && (GetFileAttributesW(wide(p).c_str())&FILE_ATTRIBUTE_DIRECTORY)) return leveldb::Status::OK();
    return error(p,code);
  }
  leveldb::Status RemoveDir(const std::string& p) override { return RemoveDirectoryW(wide(p).c_str())?leveldb::Status::OK():error(p); }
  leveldb::Status DeleteDir(const std::string& p) override { return RemoveDir(p); }
  leveldb::Status GetFileSize(const std::string& p,uint64_t* out) override {
    WIN32_FILE_ATTRIBUTE_DATA data;
    if(!GetFileAttributesExW(wide(p).c_str(),GetFileExInfoStandard,&data)) return error(p);
    *out=(static_cast<uint64_t>(data.nFileSizeHigh)<<32)|data.nFileSizeLow;
    return leveldb::Status::OK();
  }
  leveldb::Status RenameFile(const std::string& a,const std::string& b) override {
    return MoveFileExW(wide(a).c_str(),wide(b).c_str(),MOVEFILE_REPLACE_EXISTING)?leveldb::Status::OK():error(a);
  }
  leveldb::Status LockFile(const std::string& p,leveldb::FileLock** out) override {
    *out=nullptr; HANDLE h=CreateFileW(wide(p).c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
      nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h==INVALID_HANDLE_VALUE) return error(p,GetLastError(),true);
    OVERLAPPED ov={};
    if(!LockFileEx(h,LOCKFILE_EXCLUSIVE_LOCK|LOCKFILE_FAIL_IMMEDIATELY,0,MAXDWORD,MAXDWORD,&ov)) {
      auto s=error(p,GetLastError(),true); CloseHandle(h); return s;
    }
    try { *out=new Lock(h); } catch(...) { CloseHandle(h); throw; }
    return leveldb::Status::OK();
  }
  leveldb::Status UnlockFile(leveldb::FileLock* l) override {
    auto* lock=static_cast<Lock*>(l); OVERLAPPED ov={};
    auto s=UnlockFileEx(lock->file.h,0,MAXDWORD,MAXDWORD,&ov)?leveldb::Status::OK():error("unlock");
    delete lock; return s;
  }
  leveldb::Status NewLogger(const std::string& p,leveldb::Logger** out) override {
    *out=nullptr; FILE* f=_wfopen(wide(p).c_str(),L"w");
    if(!f) return leveldb::Status::IOError(p,"_wfopen failed");
    try { *out=new Logger(f); } catch(...) { std::fclose(f); throw; }
    return leveldb::Status::OK();
  }
};
}
leveldb::Env* ml_windows_env() { static Utf8Env env; return &env; }
#endif

﻿#include "gdemu.h"

#include "./emulator/emu.h"
#include "./emulator/fifo.h"
#include "./emulator/fileio.h"
#include "./emulator/vm/disk.h"

#include <map>
#include <vector>

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/aes_context.hpp>

// #include <OS.hpp>
// #include <Directory.hpp>
// #include <AESContext.hpp>
#include "emulator/zlib-1.2.11/zlib.h"

using namespace godot;


static uint8_t file_buffer[DISK_BUFFER_SIZE + TRACK_BUFFER_SIZE];

PackedByteArray compress_mem(PackedByteArray source_pool)
{
  const uint8_t *src = source_pool.ptr(); // .read().ptr();
  //圧縮前のデータと
  //そのデータのサイズを準備
  //unsigned char* src = source.data();
  unsigned int sourceLen = source_pool.size();

  // compressBoundの戻り値だけメモリを確保
  uLong destLen = compressBound(sourceLen);

  std::vector< Bytef > dest;
  dest.resize(destLen);

  // compressで圧縮。
  // destLenは呼び出し時に確保したメモリ量を指定すると
  // 圧縮後の長さが帰ってくる
  int ret = compress(dest.data(), &destLen, src, sourceLen);

  if (ret != Z_OK) {
    return PackedByteArray();
  }

  // 圧縮後のデータの長さがdestLenなので
  // 配列をdestLenに切り詰める
  dest.erase(dest.begin() + destLen, dest.end());

  PackedByteArray result = PackedByteArray();
  for(uLong i = 0; i < destLen; i++)
  {
    result.append(dest[i]);
  }
  dest.clear();

  if(destLen % 16)
  {
    for(uLong i = 0; i < 16 - (destLen % 16); i++ )
    {
      result.append(0);
    }
  }

  return result;
}

PackedByteArray uncompress_mem(PackedByteArray compressed, const uLongf original_size) {

  // 元のデータのサイズだけメモリ確保
  std::vector< Bytef > dest;
  dest.resize(original_size);

  //伸長
  uLongf destLen = original_size;
  int ret = uncompress(
    dest.data(), // 伸長したデータの格納先
    &destLen,  // 格納先のメモリサイズ
    //compressed.read().ptr(), // 圧縮データ
    compressed.ptr(),
    compressed.size()  // 圧縮データのサイズ
  );

  if (ret != Z_OK)
    return PackedByteArray();

  PackedByteArray result = PackedByteArray();
  for(uLong i = 0; i < destLen; i++)
  {
    result.append(dest[i]);
  }
  dest.clear();

  return result;
}


void decrypt_disk(const _TCHAR *path)
{
    _TCHAR encrypt_path[_MAX_PATH];
    strcpy(encrypt_path, path);
    strcat(encrypt_path, ENCRYPT_FILE_EXT);

    String dec_path = String(path);
    String enc_path = String(encrypt_path);

    // 暗号化ファイルを読むよ
    //Ref<File> enc_file = File::_new();
    Ref<FileAccess> enc_file = FileAccess::open(enc_path, FileAccess::ModeFlags::READ);
    // 4バイトサイズ
    int64_t dat_size = enc_file->get_32();
    // 16バイトIV
    PackedByteArray iv = enc_file->get_buffer(16);
    // 残りが圧縮＆暗号化ファイル
    PackedByteArray enc_dat = enc_file->get_buffer(enc_file->get_length() - 20);
    enc_file->close();

    // まずは先に暗号化を解く
    // Ref<AESContext> aes = AESContext::_new();
    String aesKey = OS::get_singleton()->get_user_data_dir() + String("auNlxwzAlC6D0b6q");
    String aesKeySha = aesKey.sha256_text();
    CharString aesKeyShaStr = aesKeySha.ascii();
    const char* pAes = aesKeyShaStr.get_data(); // .alloc_c_string();
    PackedByteArray keyp;
    for(int i = 0; i < 32; i++)
    {
        keyp.append(pAes[i]);
    }
    Ref<AESContext> aes; // = memnew(AESContext);
    aes.instantiate();
    aes->start(AESContext::Mode::MODE_CBC_DECRYPT, keyp, iv);
    PackedByteArray decrypted = aes->update(enc_dat);
    aes->finish();

    // 次に解凍する
    auto dec_data = uncompress_mem(decrypted, (uLongf)dat_size);

    // 解凍したものを書いてみる(読み終わったら消す事)
    Ref<FileAccess> dec_file = FileAccess::open(dec_path, FileAccess::ModeFlags::WRITE);
    dec_file->store_buffer(dec_data);
    dec_file->flush();
    dec_file->close();
}

void encrypt_disk(const _TCHAR *path)
{
    _TCHAR dest_path[_MAX_PATH];
    FILEIO* fio = new FILEIO();
    fio->Fopen(path, FILEIO_READ_BINARY);
    fio->Fseek(0, FILEIO_SEEK_END);
    uint32_t total_size = fio->Ftell(), offset = 0;
    fio->Fseek(0 , SEEK_SET);
    fio->Fread(file_buffer, total_size, 1);
    fio->Fclose();
    delete fio;

    PackedByteArray orig_data;
    for(uint32_t i = 0; i < total_size; i++)
    {
        orig_data.append(file_buffer[i]);
    }

    Ref<AESContext> aes; // = memnew(AESContext);
    aes.instantiate();

    // キーはユーザー名を含むディレクトリ+αとする(テキトー)
    String aesKey = OS::get_singleton()->get_user_data_dir() + "auNlxwzAlC6D0b6q";
    String aesKeySha = aesKey.sha256_text();
    CharString aesKeyShaStr = aesKeySha.ascii();
    const char* pAes = aesKeyShaStr.get_data(); // .alloc_c_string();
    PackedByteArray keyp;
    for(int i = 0; i < 32; i++)
    {
        keyp.append(pAes[i]);
    }
    //api->godot_free(pAes);

    PackedByteArray iv;
    for(int i = 0; i < 16; i++)
    {
        iv.append(rand() & 0xff);
    }

    auto comp_data = compress_mem(orig_data);

    aes->start(AESContext::Mode::MODE_CBC_ENCRYPT, keyp, iv);
    PackedByteArray encrypted = aes->update(comp_data);
    aes->finish();

    strcpy(dest_path, path);
    strcat(dest_path, ENCRYPT_FILE_EXT);

    fio = new FILEIO();
    fio->Fopen(dest_path, FILEIO_WRITE_BINARY);
    fio->FputInt32(orig_data.size());
    fio->Fwrite(iv.ptr(), iv.size(), 1);
    fio->Fwrite(encrypted.ptr(), encrypted.size(), 1);
    fio->Fclose();
    delete fio;

    // オリジナルファイルを消す
    FILEIO::RemoveFile(path);
}

String resdisk_to_userdir(String path, bool isEncrypt)
{
    Error error;
    String abs_path;

    // 指定ファイルがあるか？
    // Ref<DirAccess> dir; 
    String out_path = path.replace("res://", "user://");
    String encrypt_out_path = out_path + ENCRYPT_FILE_EXT;

    // 既にある場合はコピーせずに戻す
    if(isEncrypt && FileAccess::file_exists(encrypt_out_path))
    {
      // 暗号化ファイルの拡張子を消して返す
      // Ref<File> file = File::_new();
      // file->open(encrypt_out_path, File::ModeFlags::READ);
      Ref<FileAccess> file = FileAccess::open(encrypt_out_path, FileAccess::ModeFlags::READ);
      abs_path = file->get_path_absolute();
      file->flush();
      file->close();
      return abs_path.substr(0, abs_path.length() - 4);   // .binを消す
    } else if(!isEncrypt && FileAccess::file_exists(out_path))
    {
      // 非暗号化ファイルをそのまま返す
      // Ref<File> file = File::_new();
      // file->open(out_path, File::ModeFlags::READ);
      Ref<FileAccess> file = FileAccess::open(out_path, FileAccess::ModeFlags::READ);
      abs_path = file->get_path_absolute();
      file->close();
      return abs_path.substr(0, abs_path.length());
    }

    Ref<FileAccess> res_file = FileAccess::open(path, FileAccess::ModeFlags::READ);
    if(res_file->get_error() != Error::OK)
    {
        return "";
    }
    PackedByteArray data = res_file->get_buffer(res_file->get_length());
    res_file->close();

    // 無い場合はまずコピー
    // Ref<File> out_file = File::_new();
    // error = out_file->open(out_path, File::ModeFlags::WRITE);
    Ref<FileAccess> out_file = FileAccess::open(out_path, FileAccess::ModeFlags::WRITE);
    String orig_out_path = out_file->get_path_absolute();
    if(out_file->get_error() != Error::OK)
    {
        return "";
    }
    out_file->store_buffer(data);
    abs_path = out_file->get_path_absolute();
    out_file->flush();
    out_file->close();

    if(isEncrypt)
    {
      // コピーしたものを暗号化
      // char* abs_cpath = abs_path.alloc_c_string();
      CharString cpathStr = abs_path.ascii();
      const char* abs_cpath = (const char*)cpathStr.get_data(); // .alloc_c_string();
      encrypt_disk(abs_cpath);
      // api->godot_free(abs_cpath);

      // 暗号化前ファイルの絶対パスを拾って返す
      // Ref<File> file = File::_new();

      // そしてコピーしたオリジナルファイルを消す
      DirAccess::remove_absolute(out_path);
    }

    return orig_out_path;
}

void GDEmu::open_floppy_disk(int drv, String file_path, int bank)
{
    String abs_path = resdisk_to_userdir(file_path, DISK::is_encrypt);

    // char* cpath  = abs_path.alloc_c_string();
    CharString cpathStr = abs_path.ascii();
    const char* cpath = cpathStr.get_data(); // .alloc_c_string();

    printf("open floppy: %d %s (%d)\n", drv, cpath, bank);

    emu->open_floppy_disk(drv, cpath, bank);

    //if(cpath)
    //    api->godot_free(cpath);
}

int GDEmu::is_floppy_disk_accessed()
{
    return emu->is_floppy_disk_accessed();
}

void GDEmu::set_drivea_path(String path)
{
    drivea_path = path;
}
void GDEmu::set_driveb_path(String path)
{
    driveb_path = path;
}

String GDEmu::get_drivea_path()
{
    return drivea_path;
}

String GDEmu::get_driveb_path()
{
    return driveb_path;
}

#if USE_HARD_DISK
void GDEmu::open_hard_disk(int drv, String file_path)
{
    String abs_path = resdisk_to_userdir(file_path, false);

    //char* cpath  = abs_path.alloc_c_string();
    CharString cpathStr = abs_path.ascii();
    const char* cpath = cpathStr.get_data(); // .alloc_c_string();

    printf("open harddisk: %d %s\n", drv, cpath);

    emu->open_hard_disk(drv, cpath);

    //if(cpath)
    //    api->godot_free(cpath);
}
#endif

#if USE_CART
void GDEmu::open_cart(int drv, String str)
{
#ifdef USE_CART
    String abs_path = resdisk_to_userdir(str, false);
    // char* cpath  = abs_path.alloc_c_string();
    CharString cpathStr = abs_path.ascii();
    const char* cpath = cpathStr.get_data(); // .alloc_c_string();
    printf("open cart: %d %s\n", drv, cpath);

    emu->open_cart(drv, cpath);

//    if(cpath)
//        api->godot_free(cpath);
#endif
}
#endif

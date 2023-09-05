//
// _RevLoader_cc_
//
// Copyright (C) 2017-2023 Tactical Computing Laboratories, LLC
// All Rights Reserved
// contact@tactcomplabs.com
//
// See LICENSE in the top level directory for licensing details
//

#include "../include/RevLoader.h"
#include "RevLoader.h"

RevLoader::RevLoader( std::string Exe, std::string Args,
                      RevMem *Mem, SST::Output *Output )
  : exe(Exe), args(Args), mem(Mem), output(Output),
    RV32Entry(0x00l), RV64Entry(0x00ull) {
  if( !LoadElf() )
    output->fatal(CALL_INFO, -1, "Error: failed to load executable into memory\n");
  InitStaticMem();
}

RevLoader::~RevLoader(){
}

bool RevLoader::IsElf( const Elf64_Ehdr eh64 ){
  if( (eh64).e_ident[0] == 0x7f &&
      (eh64).e_ident[1] == 'E'  &&
      (eh64).e_ident[2] == 'L'  &&
      (eh64).e_ident[3] == 'F' )
    return true;

  return false;
}

bool RevLoader::IsRVElf32( const Elf64_Ehdr eh64 ){
  if( IsElf(eh64) && (eh64).e_ident[4] == 1 )
    return true;
  return false;
}

bool RevLoader::IsRVElf64( const Elf64_Ehdr eh64 ){
  if( IsElf(eh64) && (eh64).e_ident[4] == 2 )
    return true;
  return false;
}

bool RevLoader::IsRVLittle( const Elf64_Ehdr eh64 ){
  if( IsElf(eh64) && (eh64).e_ident[5] == 1 )
    return true;
  return false;
}

bool RevLoader::IsRVBig( const Elf64_Ehdr eh64 ){
  if( IsElf(eh64) && (eh64).e_ident[5] == 2 )
    return true;
  return false;
}

// breaks the write into cache line chunks
bool RevLoader::WriteCacheLine(uint64_t Addr, size_t Len, void *Data){
  if( Len == 0 ){
    // nothing to do here, move along
    return true;
  }

  // calculate the cache line size
  unsigned lineSize = mem->getLineSize();
  if( lineSize == 0 ){
    // default to 64byte cache lines
    lineSize = 64;
  }

  // begin writing the data, if we have a small write
  // then dispatch the write as normal.  Otherwise,
  // block the writes as cache lines
  if( Len < lineSize ){
    // one cache line to write, dispatch it
    return mem->WriteMem(0,Addr,Len,Data);
  }

  // calculate the base address of the first cache line
  size_t Total = 0;
  bool done = false;
  uint64_t BaseCacheAddr = Addr;
  while( !done ){
    if( (BaseCacheAddr%(uint64_t)(lineSize)) == 0 ){
      done = true;
    }else{
      BaseCacheAddr-=1;
    }
  }

  // write the first cache line
  size_t TmpSize = (size_t)((BaseCacheAddr+lineSize)-Addr);
  uint64_t TmpData = (uint64_t)(Data);
  uint64_t TmpAddr = Addr;
  if( !mem->WriteMem(0,TmpAddr,TmpSize,(void *)(TmpData)) ){
    output->fatal(CALL_INFO, -1, "Error: Failed to perform cache line write\n" );
  }

  TmpAddr += (uint64_t)(TmpSize);
  TmpData += TmpSize;
  Total += TmpSize;

  // no perform the remainder of the writes
  do{
    if( (Len-Total) > lineSize ){
      // setup another full cache line write
      TmpSize = lineSize;
    }else{
      // this is probably the final write operation
      TmpSize = (Len-Total);
    }

    if( !mem->WriteMem(0, TmpAddr, TmpSize, (void *)(TmpData)) ){
      output->fatal(CALL_INFO, -1, "Error: Failed to perform cache line write\n" );
    }

    // incrememnt the temp counters
    TmpAddr += (uint64_t)(TmpSize);
    TmpData += TmpSize;
    Total += TmpSize;

  }while( Total < Len );

  return true;
}

// Elf32_Ehdr, Elf32_Phdr, Elf32_Shdr, Elf32_Sym, from_le
bool RevLoader::LoadElf32(char *membuf, size_t sz){
  // Parse the ELF header
  Elf32_Ehdr *eh = (Elf32_Ehdr *)(membuf);

  // Parse the program headers
  Elf32_Phdr *ph = (Elf32_Phdr *)(membuf + eh->e_phoff);

  // Parse the section headers
  Elf32_Shdr* sh = (Elf32_Shdr*)(membuf + eh->e_shoff);
  char *shstrtab = membuf + sh[eh->e_shstrndx].sh_offset;

  // Store the entry point of the program
  RV32Entry = eh->e_entry;

  // Add memory segments for each program header
  for (unsigned i = 0; i < eh->e_phnum; i++) {
    if( sz < ph[i].p_offset + ph[i].p_filesz ){
      output->fatal(CALL_INFO, -1, "Error: RV32 Elf is unrecognizable\n" );
    }
    // Add a memory segment for the program header
    mem->AddMemSeg(ph[i].p_paddr, ph[i].p_filesz, true);
  }

  uint64_t StaticDataEnd = 0; 
  uint64_t BSSEnd = 0; 
  uint64_t DataEnd = 0; 
  uint64_t TextEnd = 0; 
  // Add memory segments for each section header 
  // - This should automatically handle overlap and not add segments
  //   that are already there from program headers
  for (unsigned i = 0; i < eh->e_shnum; i++) {
    // check if the section header name is bss
    if( strcmp(shstrtab + sh[i].sh_name, ".bss") == 0 ){
      BSSEnd = sh[i].sh_addr + sh[i].sh_size;
    }
    if( strcmp(shstrtab + sh[i].sh_name, ".text") == 0 ){
      TextEnd = sh[i].sh_addr + sh[i].sh_size;
    }
    if( strcmp(shstrtab + sh[i].sh_name, ".data") == 0 ){
      DataEnd = sh[i].sh_addr + sh[i].sh_size;
    }
    mem->AddMemSeg(sh[i].sh_addr, sh[i].sh_size, true);
  }
  // If BSS exists, static data ends after it
  if( BSSEnd > 0 ){
    StaticDataEnd = BSSEnd;
  } else if ( DataEnd > 0 ){
    // BSS Doesn't exist, but data does
    StaticDataEnd = DataEnd;
  } else if ( TextEnd > 0 ){
    // Text is last resort 
    StaticDataEnd = TextEnd;
  } else {
    // Can't find any (Text, BSS, or Data) sections
    output->fatal(CALL_INFO, -1, "Error: No text, data, or bss sections --- RV64 Elf is unrecognizable\n" );
  }

  // Check that the ELF file is valid
  if( sz < eh->e_phoff + eh->e_phnum * sizeof(*ph) )
    output->fatal(CALL_INFO, -1, "Error: RV32 Elf is unrecognizable\n" );

  // Write the program headers to memory
  elfinfo.phnum = eh->e_phnum;
  elfinfo.phent = sizeof(Elf32_Phdr);
  elfinfo.phdr  = eh->e_phoff;
  elfinfo.phdr_size = eh->e_phnum * sizeof(Elf32_Phdr);

  // set the first stack pointer
  uint32_t sp = mem->GetStackTop() - (uint32_t)(elfinfo.phdr_size);
  WriteCacheLine(sp,elfinfo.phdr_size,(void *)(ph));
  mem->SetStackTop(sp);

  // iterate over the program headers
  for( unsigned i=0; i<eh->e_phnum; i++ ){
    // Look for the loadable program headers
    if( ph[i].p_type == PT_LOAD && ph[i].p_memsz ){
      if( ph[i].p_filesz ){
        if( sz < ph[i].p_offset + ph[i].p_filesz ){
          output->fatal(CALL_INFO, -1, "Error: RV32 Elf is unrecognizable\n" );
        }
        WriteCacheLine(ph[i].p_paddr,
                      ph[i].p_filesz,
                      (uint8_t*)(membuf+ph[i].p_offset));
      }
      std::vector<uint8_t> zeros(ph[i].p_memsz - ph[i].p_filesz);
      WriteCacheLine(ph[i].p_paddr + ph[i].p_filesz,
                     ph[i].p_memsz - ph[i].p_filesz,
                     &zeros[0]);
    }
  }

  // Check that the ELF file is valid
  if( sz < eh->e_shoff + eh->e_shnum * sizeof(*sh) )
    output->fatal(CALL_INFO, -1, "Error: RV32 Elf is unrecognizable\n" );

  if( eh->e_shstrndx >= eh->e_shnum )
    output->fatal(CALL_INFO, -1, "Error: RV32 Elf is unrecognizable\n" );

  if( sz < sh[eh->e_shstrndx].sh_offset + sh[eh->e_shstrndx].sh_size )
    output->fatal(CALL_INFO, -1, "Error: RV32 Elf is unrecognizable\n" );

  unsigned strtabidx = 0;
  unsigned symtabidx = 0;

  // Iterate over every section header
  for( unsigned i=0; i<eh->e_shnum; i++ ){
    // If the section header is empty, skip it
    if( sh[i].sh_type & SHT_NOBITS )
      continue;
    if( sz < sh[i].sh_offset + sh[i].sh_size )
      output->fatal(CALL_INFO, -1, "Error: RV32 Elf is unrecognizable\n" );
    // Find the string table index
    if( strcmp(shstrtab + sh[i].sh_name, ".strtab") == 0 )
      strtabidx = i;
    // Find the symbol table index
    if( strcmp(shstrtab + sh[i].sh_name, ".symtab") == 0 )
      symtabidx = i;
  }

  // If the string table index and symbol table index are valid (NonZero)
  if( strtabidx && symtabidx ){
    // If there is a string table and symbol table, add them as valid memory
    mem->AddMemSeg(sh[strtabidx].sh_addr, sh[strtabidx].sh_size, true);
    mem->AddMemSeg(sh[symtabidx].sh_addr, sh[symtabidx].sh_size, true);
    // Parse the string table
    char *strtab = membuf + sh[strtabidx].sh_offset;
    Elf32_Sym* sym = (Elf32_Sym*)(membuf + sh[symtabidx].sh_offset);
    // Iterate over every symbol in the symbol table
    for( unsigned i=0; i<sh[symtabidx].sh_size/sizeof(Elf32_Sym); i++ ){
      // Calculate the maximum length of the symbol
      unsigned maxlen = sh[strtabidx].sh_size - sym[i].st_name;
      if( sym[i].st_name >= sh[strtabidx].sh_size )
        output->fatal(CALL_INFO, -1, "Error: RV32 Elf is unrecognizable\n" );
      if( strnlen(strtab + sym[i].st_name, maxlen) >= maxlen )
        output->fatal(CALL_INFO, -1, "Error: RV32 Elf is unrecognizable\n" );
      // Add the symbol to the symbol table
      symtable[strtab+sym[i].st_name] = sym[i].st_value;
    }
  }

  // Initialize the heap
  mem->InitHeap(StaticDataEnd);

  return true;
}

bool RevLoader::LoadElf64(char *membuf, size_t sz){
  // Parse the ELF header
  Elf64_Ehdr *eh = (Elf64_Ehdr *)(membuf);

  // Parse the program headers
  Elf64_Phdr *ph = (Elf64_Phdr *)(membuf + eh->e_phoff);

  // Parse the section headers
  Elf64_Shdr* sh = (Elf64_Shdr*)(membuf + eh->e_shoff);
  char *shstrtab = membuf + sh[eh->e_shstrndx].sh_offset;

  // Store the entry point of the program
  RV64Entry = eh->e_entry;

  // Add memory segments for each program header
  for (unsigned i = 0; i < eh->e_phnum; i++) {
    if( sz < ph[i].p_offset + ph[i].p_filesz ){
      output->fatal(CALL_INFO, -1, "Error: RV64 Elf is unrecognizable\n" );
    }
    // Add a memory segment for the program header
    mem->AddMemSeg(ph[i].p_paddr, ph[i].p_filesz, true);
  }

  uint64_t StaticDataEnd = 0; 
  uint64_t BSSEnd = 0; 
  uint64_t DataEnd = 0; 
  uint64_t TextEnd = 0; 
  // Add memory segments for each section header 
  // - This should automatically handle overlap and not add segments
  //   that are already there from program headers
  for (unsigned i = 0; i < eh->e_shnum; i++) {
    // check if the section header name is bss
    if( strcmp(shstrtab + sh[i].sh_name, ".bss") == 0 ){
      BSSEnd = sh[i].sh_addr + sh[i].sh_size;
    }
    if( strcmp(shstrtab + sh[i].sh_name, ".text") == 0 ){
      TextEnd = sh[i].sh_addr + sh[i].sh_size;
    }
    if( strcmp(shstrtab + sh[i].sh_name, ".data") == 0 ){
      DataEnd = sh[i].sh_addr + sh[i].sh_size;
    }
    mem->AddMemSeg(sh[i].sh_addr, sh[i].sh_size, true);
  }
  // If BSS exists, static data ends after it
  if( BSSEnd > 0 ){
    StaticDataEnd = BSSEnd;
  } else if ( DataEnd > 0 ){
    // BSS Doesn't exist, but data does
    StaticDataEnd = DataEnd;
  } else if ( TextEnd > 0 ){
    // Text is last resort 
    StaticDataEnd = TextEnd;
  } else {
    // Can't find any (Text, BSS, or Data) sections
    output->fatal(CALL_INFO, -1, "Error: No text, data, or bss sections --- RV64 Elf is unrecognizable\n" );
  }

  // Check that the ELF file is valid
  if( sz < eh->e_phoff + eh->e_phnum * sizeof(*ph) )
    output->fatal(CALL_INFO, -1, "Error: RV64 Elf is unrecognizable\n" );

  // Write the program headers to memory
  elfinfo.phnum = eh->e_phnum;
  elfinfo.phent = sizeof(Elf64_Phdr);
  elfinfo.phdr  = eh->e_phoff;
  elfinfo.phdr_size = eh->e_phnum * sizeof(Elf64_Phdr);

  // set the first stack pointer
  uint64_t sp = mem->GetStackTop() - (uint64_t)(elfinfo.phdr_size);
  WriteCacheLine(sp,elfinfo.phdr_size,(void *)(ph));
  mem->SetStackTop(sp);


  // iterate over the program headers
  for( unsigned i=0; i<eh->e_phnum; i++ ){
    // Look for the loadable headers
    if( ph[i].p_type == PT_LOAD && ph[i].p_memsz ){
      if( ph[i].p_filesz ){
        if( sz < ph[i].p_offset + ph[i].p_filesz ){
          output->fatal(CALL_INFO, -1, "Error: RV64 Elf is unrecognizable\n" );
        }
        WriteCacheLine(ph[i].p_paddr,
                      ph[i].p_filesz,
                      (uint8_t*)(membuf+ph[i].p_offset));
      }
      std::vector<uint8_t> zeros(ph[i].p_memsz - ph[i].p_filesz);
      WriteCacheLine(ph[i].p_paddr + ph[i].p_filesz,
                     ph[i].p_memsz - ph[i].p_filesz,
                     &zeros[0]);
    }
  }

  // Check that the ELF file is valid
  if( sz < eh->e_shoff + eh->e_shnum * sizeof(*sh) )
    output->fatal(CALL_INFO, -1, "Error: RV64 Elf is unrecognizable\n" );

  if( eh->e_shstrndx >= eh->e_shnum )
    output->fatal(CALL_INFO, -1, "Error: RV64 Elf is unrecognizable\n" );

  if( sz < sh[eh->e_shstrndx].sh_offset + sh[eh->e_shstrndx].sh_size )
    output->fatal(CALL_INFO, -1, "Error: RV64 Elf is unrecognizable\n" );

  unsigned strtabidx = 0;
  unsigned symtabidx = 0;

  // Iterate over every section header
  for( unsigned i=0; i<eh->e_shnum; i++ ){
    mem->AddMemSeg(sh[i].sh_addr, sh[i].sh_size, true);
    // If the section header is empty, skip it
    if( sh[i].sh_type & SHT_NOBITS )
      continue;
    if( sz < sh[i].sh_offset + sh[i].sh_size ){
      output->fatal(CALL_INFO, -1, "Error: RV64 Elf is unrecognizable\n" );
    }
    // Find the string table index
    if( strcmp(shstrtab + sh[i].sh_name, ".strtab") == 0 )
      strtabidx = i;
    // Find the symbol table index
    if( strcmp(shstrtab + sh[i].sh_name, ".symtab") == 0 )
      symtabidx = i;
  }

  // If the string table index and symbol table index are valid (NonZero)
  if( strtabidx && symtabidx ){
    // If there is a string table and symbol table, add them as valid memory
    mem->AddMemSeg(sh[strtabidx].sh_addr, sh[strtabidx].sh_size, true);
    mem->AddMemSeg(sh[symtabidx].sh_addr, sh[symtabidx].sh_size, true);
    // Parse the string table
    char *strtab = membuf + sh[strtabidx].sh_offset;
    Elf64_Sym* sym = (Elf64_Sym*)(membuf + sh[symtabidx].sh_offset);
    // Iterate over every symbol in the symbol table
    for( unsigned i=0; i<sh[symtabidx].sh_size/sizeof(Elf64_Sym); i++ ){
      // Calculate the maximum length of the symbol
      unsigned maxlen = sh[strtabidx].sh_size - sym[i].st_name;
      if( sym[i].st_name >= sh[strtabidx].sh_size )
        output->fatal(CALL_INFO, -1, "Error: RV64 Elf is unrecognizable\n" );
      if( strnlen(strtab + sym[i].st_name, maxlen) >= maxlen )
        output->fatal(CALL_INFO, -1, "Error: RV64 Elf is unrecognizable\n" );
      // Add the symbol to the symbol table
      symtable[strtab+sym[i].st_name] = sym[i].st_value;
    }
  }

  // Initialize the heap after the bss section header
  mem->InitHeap(StaticDataEnd);

  return true;
}

std::string RevLoader::GetArgv(unsigned entry){
  if( entry > (argv.size()-1) )
    return "";

  return argv[entry];
}

bool RevLoader::LoadProgramArgs(){
  // argv[0] = program name
  argv.push_back(exe);

  // split the rest of the arguments into tokens
  splitStr(args,' ',argv);

  if( argv.size() == 0 ){
    output->fatal(CALL_INFO, -1, "Error: failed to initialize the program arguments\n");
    return false;
  }

  // load the program args into memory
  uint64_t sp = 0x00ull;
  for( unsigned i=0; i<argv.size(); i++ ){
    output->verbose(CALL_INFO,6,0,
                    "Loading program argv[%d] = %s\n", i, argv[i].c_str() );
    sp = mem->GetStackTop();
    char tmpc[argv[i].size() + 1];
    argv[i].copy(tmpc,argv[i].size()+1);
    tmpc[argv[i].size()] = '\0';
    size_t len = argv[i].size() + 1;
    // std::cout << "Setting sp: 0x" << std::hex << sp << std::endl;
    sp -= (uint64_t)(len);
    // Align stack pointer on a 16-byte boundary per the RISC-V ABI
    sp &= ~0xF;
    // std::cout << "Setting sp: 0x" << std::hex << sp << std::endl;
    mem->SetStackTop(sp);
    //mem->WriteMem(mem->GetStackTop(),len,(void *)(&tmpc));
    WriteCacheLine(mem->GetStackTop(),len,(void *)(&tmpc));
  }

  return true;
}

void RevLoader::splitStr(const std::string& s,
                         char c,
                         std::vector<std::string>& v){
  std::string::size_type i = 0;
  std::string::size_type j = s.find(c);

  while (j != std::string::npos) {
    v.push_back(s.substr(i, j-i));
    i = ++j;
    j = s.find(c, j);
    if (j == std::string::npos)
      v.push_back(s.substr(i, s.length()));
  }
}

bool RevLoader::LoadElf(){
  // open the target file
  int fd = open(exe.c_str(), O_RDONLY);
  struct stat FileStats;
  if( fstat(fd,&FileStats) < 0 )
    output->fatal(CALL_INFO, -1, "Error: failed to stat executable file: %s\n", exe.c_str() );

  size_t FileSize = FileStats.st_size;

  // map the executable into memory
  char *membuf = (char *)(mmap(NULL,FileSize, PROT_READ, MAP_PRIVATE, fd, 0));
  if( membuf == MAP_FAILED )
    output->fatal(CALL_INFO, -1, "Error: failed to map executable file: %s\n", exe.c_str() );

  // close the target file
  close(fd);

  // check the size of the elf header
  if( FileSize < sizeof(Elf64_Ehdr) )
    output->fatal(CALL_INFO, -1, "Error: Elf header is unrecognizable\n" );

  const Elf64_Ehdr* eh64 = (const Elf64_Ehdr*)(membuf);
  if( !IsRVElf32(*eh64) && !IsRVElf64(*eh64) )
    output->fatal(CALL_INFO, -1, "Error: Cannot determine Elf32 or Elf64 from header\n" );

  if( !IsRVLittle(*eh64) )
    output->fatal(CALL_INFO, -1, "Error: Not in little endian format\n" );

  if( IsRVElf32(*eh64) ){
    if( !LoadElf32(membuf,FileSize) )
      output->fatal(CALL_INFO, -1, "Error: could not load Elf32 binary\n" );
  }else{
    if( !LoadElf64(membuf,FileSize) )
      output->fatal(CALL_INFO, -1, "Error: could not load Elf64 binary\n" );
  }

  // unmap the file
  munmap( membuf, FileSize );

  // print the symbol table entries
  std::map<std::string,uint64_t>::iterator it = symtable.begin();
  while( it != symtable.end() ){
    // create another map to allow tracer to lookup symbols
    tracer_symbols.emplace(it->second, it->first);
    output->verbose(CALL_INFO,6,0,
                    "Symbol Table Entry [%s:0x%" PRIx64 "]\n",
                    it->first.c_str(), it->second );
    it++;
  }

  /// load the program arguments
  if( !LoadProgramArgs() )
    return false;

  // Initiate a memory fence in order to ensure that the entire ELF
  // infrastructure is loaded
  mem->FenceMem(0);

  return true;
}

uint64_t RevLoader::GetSymbolAddr(std::string Symbol){
  uint64_t tmp = 0x00ull;
  if( symtable.find(Symbol) != symtable.end() ){
    tmp = symtable[Symbol];
  }
  return tmp;
}

void RevLoader::InitStaticMem(){
  // if( mem->GetMemSegs().size() != 1 ){
  //   output->fatal(CALL_INFO, 99, "Loader Error: Attempting to initialize static memory however there is either more or less than 1 memory segment\n");
  //   return;
  // } else {

  //   mem->SetHeapStart(StaticDataEnd + 1);
  //   mem->SetHeapEnd(StaticDataEnd + 1);
    return;
  // }
}

std::map<uint64_t, std::string> *SST::RevCPU::RevLoader::GetTraceSymbols()
{
    return &tracer_symbols;
}

// EOF

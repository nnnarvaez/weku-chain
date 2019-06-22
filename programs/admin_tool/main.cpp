#include <weku/chain/block_log.hpp>
#include <fstream>
#include <iostream>
#include <fc/io/raw.hpp>
#include <fc/io/json.hpp>

#include <boost/program_options.hpp>

using namespace std;
using namespace boost::program_options;
using namespace weku::chain;

void read_head_block(string path){

    std::fstream block_stream;
    block_stream.open(path, std::ios::in | std::ios::binary);
    if(!block_stream.is_open()){
        std::cout << "open file error."<<std::endl;
        throw std::runtime_error("Could not open file.");
    }

    block_stream.seekg(0,std::ios::end);
    auto length=block_stream.tellg();
    std::cout<<"file size: "<<length<<std::endl;

    // get head block position
    uint64_t pos;
    block_stream.seekg(-sizeof(pos), std::ios::end);
    block_stream.read((char*)&pos, sizeof(pos));
    std::cout << "head block position: " << pos << std::endl;

    // read block at position
    std::pair<weku::protocol::signed_block, uint64_t> result;
    block_stream.seekg( pos, std::ios::beg);
    fc::raw::unpack( block_stream, result.first );
    result.second = uint64_t(block_stream.tellg()) + 8;

    std::cout << "second: " << result.second << std::endl;
    std::cout << "head blog time stamp: " << result.first.timestamp.to_iso_string() << std::endl;
    std::cout << fc::json::to_string(result.first)<<std::endl;

    block_stream.close();
}

int main(int argc, char* argv[]){

    options_description options("admin tool options");
    options.add_options()
            ("help", "help info")
            ("filename", value<string>(), "read block file")
            ("number", value<int>(), "read block number");

    variables_map vmap;
    store(parse_command_line(argc, argv, options), vmap);

    string path;
    int block_number = -1;
    if(vmap.count("number"))
        block_number = vmap["number"].as<int>();

    if(vmap.count("filename")){
        path = vmap["filename"].as<string>();
        signed_block b;

        block_log _block_log;
        _block_log.open(path);
        if(block_number == -1)
            b = _block_log.read_head();
        else{
            b = *_block_log.read_block_by_num(block_number);
            FC_ASSERT( b.block_num() == block_number ,
                    "Wrong block was read from block log.",
                    ( "returned", b.block_num() )( "expected", block_number ));
        }

        cout << "block id:" << b.id().str() << endl;
        cout << "block number:" << b.block_num() << endl;
        cout << fc::json::to_string(b) << endl; //to_pretty_string
    }

    if(vmap.count("help"))
        cout << options << endl;

    return 0;
}

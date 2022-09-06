
#define JSONCONS_NO_DEPRECATED
//#define JSONCONS_NO_EXCEPTIONS

#include <iomanip>
#include <iostream>
#include <vector>

#include <jsoncons/json.hpp>
#include <jsoncons_ext/cbor/cbor.hpp>
#include <jsoncons_ext/jsonpath/jsonpath.hpp>

#include <libprotoserial/data/container.hpp>

using namespace jsoncons; // for convenience

const std::vector<uint8_t> data = {
    0x9f, // Start indefinte length array
      0x83, // Array of length 3
        0x63, // String value of length 3
          0x66,0x6f,0x6f, // "foo" 
        0x44, // Byte string value of length 4
          0x50,0x75,0x73,0x73, // 'P''u''s''s'
        0xc5, // Tag 5 (bigfloat)
          0x82, // Array of length 2
            0x20, // -1
            0x03, // 3   
      0x83, // Another array of length 3
        0x63, // String value of length 3
          0x62,0x61,0x72, // "bar"
        0xd6, // Expected conversion to base64
        0x44, // Byte string value of length 4
          0x50,0x75,0x73,0x73, // 'P''u''s''s'
        0xc4, // Tag 4 (decimal fraction)
          0x82, // Array of length 2
            0x38, // Negative integer of length 1
              0x1c, // -29
            0xc2, // Tag 2 (positive bignum)
              0x4d, // Byte string value of length 13
                0x01,0x8e,0xe9,0x0f,0xf6,0xc3,0x73,0xe0,0xee,0x4e,0x3f,0x0a,0xd2,
    0xff // "break"
};


int main()
{
    /* // Parse the CBOR data into a json value
    json j = cbor::decode_cbor<json>(data);

    // Pretty print
    std::cout << "(1)\n" << pretty_print(j) << "\n\n";

    // Iterate over rows
    std::cout << "(2)\n";
    for (const auto& row : j.array_range())
    {
        std::cout << row[1].as<jsoncons::byte_string>() << " (" << row[1].tag() << ")\n";
    }
    std::cout << "\n"; */

    /* // Select the third column with JSONPath
    std::cout << "(3)\n";
    json result = jsonpath::json_query(j,"$[*][2]");
    std::cout << pretty_print(result) << "\n\n";

    // Serialize back to CBOR
    std::vector<uint8_t> buffer;
    cbor::encode_cbor(j, buffer);
    std::cout << "(4)\n" << byte_string_view(buffer) << "\n\n"; */

    static_assert(type_traits::has_data_exact<const sp::bytes::value_type*, const sp::bytes>::value);
    static_assert(type_traits::has_size<sp::bytes>::value);
    static_assert(type_traits::is_byte<sp::bytes::value_type>::value);
    static_assert(type_traits::is_byte_sequence<sp::bytes>::value);
    static_assert(!type_traits::is_basic_string<sp::bytes>::value && 
        type_traits::is_back_insertable_byte_container<sp::bytes>::value);


    json j;
    std::cout << pretty_print(j) << "\n\n";


    j["cmd"] = "name";
    j["args"] = json::make_array(3);
    j["args"][0] = "arg0";
    j["args"][1] = 1.0;
    sp::bytes b = {3, 4, 5};
    j["args"][2] = json(byte_string_arg, b); //std::vector<uint8_t>{{3, 4, 5}});

    std::cout << pretty_print(j) << "\n\n";
    
    
    const auto & args = j["args"];
    std::cout << args.is_array() << std::endl;
    std::cout << args.size() << std::endl;
    std::cout << args[2].is_byte_string() << std::endl;
    std::cout << args[2].as<sp::bytes>(byte_string_arg, semantic_tag::none) << std::endl; //std::vector<sp::bytes::value_type>


    const auto & cmd = j["cmd"];
    std::cout << cmd.as<std::string>() << std::endl;
    std::cout << cmd.is_number() << std::endl;
    std::cout << cmd.is_string() << std::endl;

    //auto ins = std::back_insert_iterator<sp::bytes>(buffer);
    /* auto sink = jsoncons::bytes_sink<sp::bytes>(buffer);
    auto ins = std::back_inserter(sink);
    *ins++ = (sp::byte)0;
    *ins++ = (sp::byte)1; */

    sp::bytes buffer;
    cbor::encode_cbor(j, buffer);
    //std::cout << "(4)\n" << byte_string_view(buffer) << "\n\n";
    std::cout << buffer << std::endl;

    json jp = cbor::decode_cbor<json>(buffer);
}

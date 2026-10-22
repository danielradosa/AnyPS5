#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>
#include <vector>

struct GuestAsn1String {
    int length;
    int type;
    unsigned char* data;
    std::int64_t flags;
};

struct GuestStack {
    int num;
    char** data;
    int sorted;
    int num_alloc;
    void* comp;
};

struct GuestPkcs12 {
    GuestAsn1String* version;
    void* mac;
    void* authsafes;
};

struct GuestX509 {
    void* cert_info;
    void* sig_alg;
    void* signature;
    int valid;
    int references;
};

struct GuestEvpPkey {
    int type;
    int save_type;
    int references;
};

struct GuestName;
struct GuestBio;
struct FileStream;

extern "C" {
GuestPkcs12* APS5_VABI d2i_PKCS12_bio_nid_postfix(GuestBio*, GuestPkcs12**);
GuestPkcs12* APS5_VABI d2i_PKCS12_fp_nid_postfix(FileStream*, GuestPkcs12**);
void APS5_VABI PKCS12_free_nid_postfix(GuestPkcs12*);
void APS5_VABI PKCS12_PBE_add_nid_postfix();
int APS5_VABI PKCS12_parse_nid_postfix(GuestPkcs12*, const char*, GuestEvpPkey**, GuestX509**, GuestStack**);
GuestName* APS5_VABI X509_get_subject_name_nid_postfix(GuestX509*);
int APS5_VABI X509_NAME_get_text_by_NID_nid_postfix(GuestName*, int, char*, int);
void APS5_VABI X509_free_nid_postfix(GuestX509*);
void APS5_VABI EVP_PKEY_free_nid_postfix(GuestEvpPkey*);
int APS5_VABI sk_num_nid_postfix(const GuestStack*);
void* APS5_VABI sk_value_nid_postfix(const GuestStack*, int);
void APS5_VABI sk_pop_free_nid_postfix(GuestStack*, void (APS5_VABI*)(void*));
std::int64_t APS5_VABI ASN1_INTEGER_get_nid_postfix(const GuestAsn1String*);
GuestBio* APS5_VABI BIO_new_mem_buf_nid_postfix(void*, int);
int APS5_VABI BIO_free_nid_postfix(GuestBio*);
std::int64_t APS5_VABI BIO_ctrl_nid_postfix(GuestBio*, int, std::int64_t, void*);
std::uint64_t APS5_VABI ERR_get_error_nid_postfix();
void APS5_VABI ERR_clear_error_nid_postfix();
FileStream* APS5_VABI fopen_nid_postfix(const char*, const char*);
int APS5_VABI fclose_nid_postfix(FileStream*);
std::size_t APS5_VABI fwrite_nid_postfix(const void*, std::size_t, std::size_t, FileStream*);
}

static const char ModernHex[] =
    "308209c50201033082097b06092a864886f70d010701a082096c048209683082"
    "0964308207fa06092a864886f70d010706a08207eb308207e7020100308207e0"
    "06092a864886f70d010701305f06092a864886f70d01050d3052303106092a86"
    "4886f70d01050c3024041069b11ba212723c358495d79601ab33c90202080030"
    "0c06082a864886f70d02090500301d060960864801650304012a0410820c1112"
    "5a22f29221ed595086c22dc7808207709624017ece3328703b83e0e9ff9b3e93"
    "132b335a0d1b518b04ced4896d00300252dfb68ab744018bc0fbe488f6996222"
    "aa8b31994ec62193ec3da632b87ff5375c41a852b5375d8317b51a34a0ef6b36"
    "34c81ece5c2a595cefb658e48e36e253e1a8e46a12ae64dfb7885c8e77a7fbb0"
    "5f02e503b855272bf2718ce702414fa6164f6f392b6afa810c6953606b3af0d8"
    "fa330672de1a96e7b763dcf83ff5cd9e566e523ee241174c8142787a6f3a46e2"
    "215807ae69a5321705090f1f0634a51960f327ef52eead0b0739ed7c1b1441ab"
    "7c88e6864c0c9a1f87c7b8132216cf97b935b9141c89dd7dcada941366b6b2a1"
    "beba922e54f8d6c76d5dd7bcc7f7cd8f4b237baba04ab5efcf7436cc64b660cd"
    "d7fb2695e009e60e7bb5f5dc7cc0b4802ed472f215cfb77f5d6a5ca8fb1c195b"
    "333f409507aacc1dc76de962d04157918bc0803ffb819bfd5f27b95551808b87"
    "0611e1de252db060ff4f78199a11233d6517166eb1484789848e585e8690f01f"
    "84e07d296a7a34fa1cd02b7946730adf63f1935e111ec69987b8a3fe7a7a3d67"
    "1537dd6afd265b0a77203d1005e4314fd14855a6db7b0edbd774e6b8cbc86d45"
    "1f780625a6b13b3c711f8ccd785449423e8dcaf71709ace5a571998ec56581e0"
    "77c6a9553e44986c6d5f8a20d91453e695524a94b97750454e0c00a230f7380d"
    "68750f4fd9605aee276dd462cefa06df4026953e5d9275ed981946484d7be0a2"
    "5b8e62e15d9548866749df7ee4660a016e34394f10d3322db2f1d776e527d451"
    "749708629a853eb23698d72e25116833b997abe0cb0c1ea5bd4bc8006be9a08c"
    "cf8b014596e220fc7df1471cbb429a0759779e5808bd88c37307f4891c2d13d3"
    "ce99f869e6ba96b63cc4c475dfb8eed6310d65668f686f81e5584a9d86357cdf"
    "afef6307a903aa15f469c55c8f0d7bdae38f57774b4b70407bbfa4e605c8fc62"
    "556929a4535fd45c43367f31f8c0c169bcc12ad4fcaea4ca92d42c819d5ace64"
    "272d2bd38646607bee489a1a23c605225353f4f7a8193e041f2e1ad9ba620a97"
    "f66a77d23eac98089e83d7b3083f03b27b2c4e84fb6a6880bcd0a03a87efbca1"
    "426e4e8acd7327abc13b5e563599254647a63d8e83e5efeab60537c03b5f82a9"
    "ee42c8096b76d67517e60d1cc3f113e9eea1915f9bd796edbd3d330a4f916af3"
    "b8b14d122b62e3e86e8e5d72521134cbd8b562221615ceee544e38945f2fa687"
    "4e2f38f00834849dc869cf3f45242c0938719dbd9a63464ba93018aa8ee36e1c"
    "429049d78b5887da662770d4ac6254203c080c0d96efb5002368d6b62af18fa7"
    "0225e5dd512e4ad4f1fb78220190d17c0ddf1c8bb797efc116bcaa10506129ca"
    "c936e138edbd9dfc2c4581a0537ad079421c010911c1193ec09223d2b803ff9d"
    "ec029a9b18eecd0e4da39263f04d82b1a66d5c708f0cf0ba20db8a9791b98371"
    "3b5b24d5661871a97832b06666102264c8a5d12f2fa6fdb796eb44b1dea242bf"
    "95aa8fc5674920cf04ca75f85eb431af7bc1802fafe82c20e961566c311964b8"
    "71ee3a01125837fa6f3805c949f3e36e4a48d4157cfd83f2d58708bdf132370d"
    "ca938e76a32b2e781d689edc833a0a863a47988ae13d822966928ca440351961"
    "d988d7e311738f4b1992f1c3ecbba90874e72d6550265bbd12f86f3416b96d6c"
    "87f9198debfcd6ec453b7428c10c823de2276b75aa2a6619ede4cfb7b16beb2b"
    "c9a84b5dd95ef38bb41586d8acf563df06d1696e7a97b56e6397cf19f00e9c38"
    "ac72fd72693b4b4d2d7b8fd48a4e081032906cd2bbc0d05950786ca2a6ce88b6"
    "85581ae7e86d72aec8dba7c532a9c342bb6c36342ac1120449996142ceddfcd9"
    "04a316296356782f00068c0732b272372a3e1bcba4ea298f6c2f60fa97d5c01b"
    "4bed00a62ceda1423f629aec0d7409d2563f3605b41c10314a2fdf2a107d8532"
    "cbc97b6a1040a68177607c579ffc0bc9d9de3e722d5594e7aab9901ddc141f11"
    "604c22d061becb421fc6bc892c9e2841b6b5a627b55e8f84c3b114ae423c398e"
    "9221f064d8349b214f05db51826a7e4633f05125792c7c3f9542c74b5435cd75"
    "e9ac560df7522f65b4589404cb3deb1808e7db7e3b43caf5a4411a3e12ec827e"
    "c723a2c8fc88bb60cad5a77017dff97544c9d4a2bcb314fb065c47552a3f5309"
    "dcf9e6d83211a6b13ea1ebd609aa29e9ea4e852baf3e63cf27e3e4547bd97a49"
    "73555bdde3d8b2cec2e2d067d6d8e925a7c31b313df4a0ffcd1063edfe7dea31"
    "2049cbb1d61540ec65dd583077166634ab7b59c0d8231c141cb9447b7cd66788"
    "15f109dd9bc83f9f54ab31e18eb603fafe1bc798bc4127f7ca0d73a3fcac0d8d"
    "4a25d29fcbf1462c2c890ca470b6bd458dad15237d916cc2cbffeed0b6443819"
    "c483564dae3b44f52fd132bce2a06d205dddaf7c1068d672b2ec4ac109fd0e07"
    "133ba80bf9c07f07d07c61834e025c9e3cf169801e451b3450b525d6cea25154"
    "50912ece701bdfd1df3c8c5d05b40b831df53e5e1fabe7d787fbd2fd516cf3b3"
    "5729e6cc8a1c096a03bdce7c45364dfe91974a8112b940699cf75e75df5825f3"
    "7a03ce7ae8d417e865e2d44452fb202e1d773a45ac7fe8fb360c3bb8a300df02"
    "b8ccf0cee77c04edb6578b1d5298c7e5dc5aafa7b070206beb6a2a0c114472f6"
    "3082016206092a864886f70d010701a08201530482014f3082014b3082014706"
    "0b2a864886f70d010c0a0102a081f73081f4305f06092a864886f70d01050d30"
    "52303106092a864886f70d01050c302404109a030ef3394f9cb1e50484a81ed7"
    "76f902020800300c06082a864886f70d02090500301d06096086480165030401"
    "2a04105a6e2ef5f32f12bbc6941915beb9bc9f048190d98847fda3876e83e3a1"
    "b6444d621515afbeb44c2cbfe94af697dd8081c6cf95b4995c59d6bfec71c068"
    "1b9b20ac12ce609a76ec185d4f649e46af5039c6af8acb1ae9704eff7ad96334"
    "d0226dc8a813b97151f72f45bf13ffc088801c4dca2e92a130d4175d7ccf48b4"
    "3241168a8f35862cc51ebea847376719b1ce7b30e67ebf5e47de94455e51c3e6"
    "eb078791abbf313e301706092a864886f70d010914310a1e08006c0065006100"
    "66302306092a864886f70d01091531160414f2d9dfe5288f3f2fb25c94c4ada1"
    "656a44b799c330413031300d0609608648016503040201050004201d74054729"
    "9a5943519fa0748b70740fa43e2b8582a547401e2363f490b1777e04081f0b8a"
    "58b98c0a4d02020800";

static const char LegacyHex[] =
    "308208ea020103308208b006092a864886f70d010701a08208a10482089d3082"
    "08993082078f06092a864886f70d010706a08207803082077c02010030820775"
    "06092a864886f70d010701301c060a2a864886f70d010c0103300e0408380d98"
    "8929f3631e0202080080820748f132af215f8a80a485236a506102ceb86be441"
    "4bd279b9611f3f423970f6536f09a58e00dce70a3a76101f044e844679dabfc5"
    "5381449ca4ccc604a0b663dad54e67da87f2a1182d27d0382f1c0a4b321a4e1d"
    "1f9b3a2168c842abb9ebc687cc284be9891d5d649646704b82853d16bbf34807"
    "99b598e267dfd3fe8dc518f05ac149577960e12dee2036c1e1d9f92eed0c844f"
    "ef23a9bf10c9031ee88850f2ab6f74478ad7f8cd25a668dc35239e25256cb84f"
    "1f142391c5dca34c1b1c2eef3521496dab2005732df2d8e4bc85fc46fa1ab2a8"
    "941a2a7bf95d79b7b9cc5a398a8f631d149fc3810397d7e54dd96de589ba7677"
    "60fde0bd8e4084dd7190ab5f081923a8c68ede57fed7ef224c5921d0097481a4"
    "a738e41cb8496edf0316cf6961489d2a75cfa0c3d5f3c009d6316cc57c40a782"
    "5c0ee2dfb5036ee2de9089a9559a28fbe8fe243e9a27b2583d6ad95d534b470e"
    "710d44b27932b2d665541056fc43f499f724e925be1752f0bb03deb7c9537c8d"
    "4c985cc227fb9560a7586ba1377b4e393d97a79cecd33e7e70367d2399127212"
    "11dda8d353d96cd1091c16d1c2c25c5c3982caae1f94d59a20b3e67f7324c564"
    "73b208ff630e3454fc71104f87ab41e0d51ed4e2ce37f5dabdfc652080dd8132"
    "ac48767530a8f9f763b990de049680d70914ac8c0e9c90ac2f6271137996ead0"
    "b1410bf8973684af16dbb7a72aa494f9b4742e131e171e08960c192c696a634f"
    "a3990a55a746ce7a5459333f0c5b1305d288967832650afd13a47e36129e17d6"
    "2ea6b748baddb600a616f8db6900bf7af180e08e2ce1b060f78bff6a3f4ddfdf"
    "0ac660006847e4f12cfb5bb7ae87cd868a81f12e1d9a980f0c365d246f9afc0f"
    "5d4b0c84cd1e3c0b78ae89414def623b4c79e6fda271e8d4f4e544a1fa3ad842"
    "3d3d96a1d58c44af8e1786a9c8664504d2da8e4d016960fa2c74a65a471f48dd"
    "37c19cabecba6b2a7861ed1a4afc15147fef4de1c0d3bd70624c13da14e4969b"
    "21dfdcc5cebe3daacfb805ed739a4f39a9c04c81aef1ce8d7d0d207e1014a454"
    "25a750267e0992e9dd13919eab96208b55dbcdaa1c56b004e8a2ee6d53a1b934"
    "ff0f8934680b0a72aa3beb36f111b6a3cf2e01fc0836e4f3da8625e3fe53e042"
    "a87be659fb063c11b0fb4ea6a0abcf7e9c37f1e4ccd4405f4727320641a54ebc"
    "4aa1de560005ef8e03afb8b38b142b171f548b35735380715c1b6d64bc82e224"
    "16bf7d9f7b4245534796a37902b2ad8abeea5c83523fb5f757032e31429e3fc4"
    "7f61a03ba580ab8fcf1994c7c2b4baaf848d5adac9e9b903ba375457ee7bdcdb"
    "74e045f31865a72c24d68271bb5f8917b438db96babad99f3af9483d21c2a024"
    "e6a3cec748d883037c71d726bff2922cfd7ba150b08144752bb5f620f9741f80"
    "47a62e99a72a7ce4352100b60da0103467c31f89ec5429ae67173cbdf6ae73f0"
    "cbb97be9cd7f7e188de82f084c96acdbff464c551c6a1a83c6fae7873e44d708"
    "64352967088ea3af119fa7ac4a18fbfdb9144a420dcd1f343df64d33edc15a6e"
    "1e3aee17c0b005af252b4efce204f1751d41b41df2da132f603e82655636f18e"
    "306a4a6eedb549b30172cc6606c3a5ed3db6a612318cf64f3959b3edf5a295bc"
    "19be97deda64505b3ee256922dd83ee48721f79c8d4bfa573475061dd737cac2"
    "430ba294b340caf58046e3f0adba7b8843bb21326dc435a4530c1056e615036f"
    "acc9dfba0f661e5b8688b3c67899073d50947f92fb97aaefb6b540a360ce359c"
    "a68962d50353f9e7ae03c5853d7d21fb76eb63027cb156ab1ea7039d63ec7358"
    "28266fb0eac4782f6c493dec7d0579d4aa82cbf4e0ef2bbcf07a817cf478a522"
    "2835fa21d872305154fa1e5a65553f12cdbdc3953cff3bf685981a158df14425"
    "600ac0aff399c6c1a8c77ce3ff12da61933a35af6bfe740d79194598a2a47a6d"
    "7890e849cad46622410f883ffa2593e98b001db73ef7fc4434f7119cdd5b2f7f"
    "87f1652bd3f953a2b502baf0fc96bf0df997e3153136ed2db585efa2c39e8ad3"
    "59f8fac04825489d9f0965a1fd66c4ea939b8e97fad2de2ae10a2b6177f0e411"
    "e81b075f4fa7fc273396e25d087da49a64ba6e2806ed44b3c306aa5c29099204"
    "d4944ec86c576941a9b5b58f4249b3d7a826ccb76e8a1382d5d30f6bc818b86a"
    "0be56dbd5b2386e263207be1608fbdbd7d540e811f3c5af180bfcb8d05d406cb"
    "4c6fe778cfb3dbe450b17d9b7b3128ade60a767b6a59bdcad67c7bb1f909edb1"
    "2aa6730a6a1283060f7616a3f59e8f207a5345b36fa338f9af3da2a9398ad36c"
    "0f98399d54ee650dfa0e140d5fcab98c8c960d9cc47ddfee520aae2ea400c110"
    "a45e4cb7a547910e1bc00a6e229ee3b4b1c59122ce6b97d73acaa488dee9be9b"
    "0ca31ee55fe1aab79fbc8c1213b8ebbc6cefa3adbf50a0d5df30929f2adae6d3"
    "8fa2c433f97a47b70a82e65a370baefcb489fdc020b29223ee39cf71e63f7a6f"
    "a34fa12c5abe8c03fc8fbc19fd70644bdd56531e8c08d0beb1f9060ab947fce3"
    "8b6d9ca2f48088eaac7ef67a607fbe51acf23ccfa3f8c9ee23da585a84705c68"
    "3219b2ab68fe905a64887ecadac7b5078c31ab15913082010206092a864886f7"
    "0d010701a081f40481f13081ee3081eb060b2a864886f70d010c0a0102a081b4"
    "3081b1301c060a2a864886f70d010c0103300e0408660fcc238b932b12020208"
    "0004819058c2bc5d6fc166adab9cfe7ffb10a99ab70ed86c906e7b2e88c9e743"
    "f0273613873fc07bccae0a1aa050be5b1a462b4e73028ab4b533956a8debe6d2"
    "20c464fe1fb3ebee921dfa4614ab0b94dc689393002f21af9c176a7b993b715c"
    "c5966ca634d9e701856611b590fdffd2c426d9a1ef9b0a46690e3e7936d59cc4"
    "ab85d5d0be1fabea99721a03dfb1f5ce1d9cf95d3125302306092a864886f70d"
    "01091531160414f2d9dfe5288f3f2fb25c94c4ada1656a44b799c33031302130"
    "0906052b0e03021a05000414e01628cc6a7601c68af227d79dfca82aa6d556d0"
    "0408b19ea7fc6981f47c02020800";

static const char EmptyHex[] =
    "30820609020103308205cf06092a864886f70d010701a08205c0048205bc3082"
    "05b8308202b706092a864886f70d010706a08202a8308202a40201003082029d"
    "06092a864886f70d010701301c060a2a864886f70d010c0103300e0408e78513"
    "fa9beb630a02020800808202704e130b399ea51a63730c22d2cb6f6feef69f9b"
    "69945efadafe55f2f5c6341ad27859fbe0bb5b78be153623f9ce8dd629bad7d1"
    "b975c98e16bceab3d62aa2caf5d7bd6e063d6d30001fcaafefaf9fd290deb256"
    "0987a544a78521c073ce689b5d17fead53f71be66ada50e6d22ee8d52da141fb"
    "e06bd43788ddc6e82536cc2cff88b19fe0bfbc9259215422648f347234f7858c"
    "f76bba27d2ab994ac0c3c16cae4f4733e29f7a4005fa48fb0230ddc64fe9fa63"
    "af12dd53f5b446089ba3b41f3b6ce9f4e8bd47d8d20a1d238bd5424dcfb72f5f"
    "4a607abf461bc719276e75894f6146476d803b7d764e1fe53ed0f079f896376c"
    "e7ca675f50a5d4ae54cc73b1a5c81432852cbf64b5cb8f66fe958fe7a5edf60e"
    "32fd891f4c89c85ba65db4a34bf989d1b1a4c489acf944a4d8fe393d1336d66c"
    "f3cde9dbe731d513a5bac726c7d01720efd837f7540a52def63c10443b6cf22a"
    "7fed7b8d44972ce70a94bce4632ad6797ba0cfcb674a59f94480c73102acb968"
    "93e635a2f9ce5efacb3b12e4dba7a152f0d5fc3da4a33514d19e23073719dd88"
    "57a28d86833d9f93a03bfd69ec65526e454ba38e7601b15734a9766160c65947"
    "17eb1715e2aacf163f79a0c11d489fcd9110834c7f75c580adbcbaa6c7511ffc"
    "062803248726c4b611124bfb0f5b7fd488d5d2bfe6c702f0e33027b979096906"
    "e85e838081e993d63dc852907e98c72b05fa31ab2369d3ea588a2e1da5c41cbf"
    "ec38321687f43debc16a6de8f876091018d1f31bcf2c6138290134c5b5888690"
    "22f754d3d122220de12cb9e37aecc894c17fcdee7a249da9c84a2af78e3d5d67"
    "de84e83cfa3591e160a32a4427cfb4ac62f63b20950254dfca3903434c308202"
    "f906092a864886f70d010701a08202ea048202e6308202e2308202de060b2a86"
    "4886f70d010c0a0102a08202a6308202a2301c060a2a864886f70d010c010330"
    "0e040835aa7c841a9f39680202080004820280d862696e02789bc21c7474b918"
    "b2edea190f186ca00a07ee74a003be839113d41dc705d75bae1a0b1a7c314df2"
    "64502ae71e8f2963eb38fffb188db5381c92c447f3ec48d1b1afdaac3b5982e5"
    "18b32fcd16dc82a448fbc24d8529b296933849ec4e50030a9d3ce6574db34454"
    "e9cd75df0698774fb40a5e637e67bd3e35a597bcfb16d062db90fa14c166aeca"
    "be0a71d926ed8adfb585464771c15ea72a9a335b18571052830db7106c560944"
    "491709618e827f9af1d46df8a0a93e70447e3465c290419f3beb004a7690b80e"
    "9858b455f692a65651c235879c9cbb1ad8351fd767dfc1d0fc4378a4baa88052"
    "ae004efdc46c29cfbc3f22a93300e416ffe54f483f9216fd533ba04697851276"
    "f4a88311289c8c25fc2b0b86e9c84cad563a061aeb68d2d7e3e6e1386df3d643"
    "539948076ff6533260d8c150af2269f2faf6c87e347c06e67ea97a5cf846dc8e"
    "74d6a221089fc9cc535b2cac73ca5554ac83087780e6a9c0ce135e64c9f18ca5"
    "e153dfdf4be9cde46c12c758dbff700502975b75a94ba7ddf4006378a25604d8"
    "67b2ea3d4a304372e27bf82a25e8bd50aabeb9541307a04b980d7043c68fa920"
    "4f19d3c02d38635a1bfaa30d76777dbf88015d4d85b4770a78b096ddbcb2d92a"
    "2de418d515e828339d63d52ce059b2c25d3f67ac289accbf03530ebf702be106"
    "086020bf59f15ea5fa8bad2eef05fa825a4bcfacd2604967d32a0a3ec43ca663"
    "246c157998faca533c2b48ee1903ed7d1abddc036344b42d8ed12f3b556dbe63"
    "f72f67677d9f07f89f4d950211f628dbeaada5b37c57432c7404aa83da6f6e81"
    "3f8e8508184d8f9c8f7901215c7fd1a0fa382cdf502c03686e9c01322c9aa6ce"
    "3b237faf43701dba5a23f783c2a0dd4e7163c43125302306092a864886f70d01"
    "091531160414a15ee446d5c7ecdb1d83e77c2134ba3dde2f6f16303130213009"
    "06052b0e03021a05000414486905166dc0bcf449e76af4517c893e3e6c585404"
    "084e5560abb0cdabdb02020800";

static const char NoMacHex[] =
    "308204b4020103308204ad06092a864886f70d010701a082049e0482049a3082"
    "04963082034506092a864886f70d010701a0820336048203323082032e308203"
    "2a060b2a864886f70d010c0a0103a08202f2308202ee060a2a864886f70d0109"
    "1601a08202de048202da308202d63082023fa003020102020103300d06092a86"
    "4886f70d01010b0500303f310b300906035504061302435a3114301206035504"
    "0a0c0b416e795053352054657374311a301806035504030c115465737420496e"
    "7465726d656469617465301e170d3236313032313138323034335a170d343531"
    "3232303138323034335a303f310b300906035504061302435a31143012060355"
    "040a0c0b416e795053352054657374311a301806035504030c116c6561662e65"
    "78616d706c652e746573743059301306072a8648ce3d020106082a8648ce3d03"
    "0107034200044ffc1a1641c450c1c6cdcc42550b3a2a642b6c2189c64756759b"
    "e716c297fb62ec5c5c7a3586bfa25ffe993067b7236a80d7dcbf6f13bf32a250"
    "790c599deaf8a38201273082012330090603551d1304023000300e0603551d0f"
    "0101ff040403020780301d0603551d0e0416041489bc02ca7723d2a1582dd79a"
    "d8bc76b2e3632818301f0603551d23041830168014b715918bba2372698f3614"
    "44e979d343a6affc243081c50603551d110481bd3081ba820e2a2e6578616d70"
    "6c652e746573748211686f73742e6578616d706c652e746573748704c0000207"
    "811261646d696e406578616d706c652e74657374861568747470733a2f2f6578"
    "616d706c652e746573742f88032a0304a43c303a310b30090603550406130243"
    "5a3110300e060355040a0c07416c74204469723119301706035504030c10616c"
    "742e6578616d706c652e74657374a021060a2b060104018237140203a0130c11"
    "75736572406578616d706c652e74657374300d06092a864886f70d01010b0500"
    "038181009068364e71e7cbdc47326569385883bd139e4825d61889c2a04bbb72"
    "0c8c4b5daca620d476e85dc93bdc7d67bdf38bd621b9a4b4e091fd324fa5e160"
    "371d868f6392b3eb6a9f77d2fce5fe14a02148c319709ebaa72aa22af2112e9b"
    "d6a4d1ba459963a324283547dd30c54e20d9d5a31a4041bb31f23d0516a83235"
    "7bbec7d23125302306092a864886f70d01091531160414f2d9dfe5288f3f2fb2"
    "5c94c4ada1656a44b799c33082014906092a864886f70d010701a082013a0482"
    "0136308201323082012e060b2a864886f70d010c0a0102a081f73081f4305f06"
    "092a864886f70d01050d3052303106092a864886f70d01050c30240410e6100b"
    "21afbc948908a63a69ed36155d02020800300c06082a864886f70d0209050030"
    "1d060960864801650304012a0410adf8b0db1d6705eb4978c18f59d26f840481"
    "90828e26708e4f44876508047d5bf8f01ac46eb623578573f18a4a3fdb98d9e8"
    "eb4c198cd90f4e95857c6d541df95d3d9890980c6b5de4ffb1838beacf9d1a49"
    "0bbce2974d8c9143c88b01dc290d21a85340696cdc2a40da8e5a4165b42231a6"
    "f1e111683b47bacd4f5bf1353761c20573887f212ca51089a395da364f360b32"
    "87cc93b9bbc6f9576bee5eefe90e572ee43125302306092a864886f70d010915"
    "31160414f2d9dfe5288f3f2fb25c94c4ada1656a44b799c3";

static void Require(bool value) {
    if (!value) std::abort();
}

template <typename TCall>
static bool Throws(TCall call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

static std::vector<unsigned char> Hex(const char* text) {
    std::vector<unsigned char> out;
    for (std::size_t index = 0; text[index] && text[index + 1]; index += 2) {
        const auto nibble = [](char c) { return c <= '9' ? c - '0' : c - 'a' + 10; };
        out.push_back(static_cast<unsigned char>(nibble(text[index]) << 4 | nibble(text[index + 1])));
    }
    return out;
}

static std::uint64_t Code(std::uint64_t library, std::uint64_t function, std::uint64_t reason) {
    return library << 24 | function << 12 | reason;
}

static std::vector<std::uint64_t> Errors() {
    std::vector<std::uint64_t> codes;
    while (const auto code = ERR_get_error_nid_postfix()) codes.push_back(code);
    return codes;
}

static GuestPkcs12* Read(const char* hex) {
    auto der = Hex(hex);
    GuestBio* bio = BIO_new_mem_buf_nid_postfix(der.data(), static_cast<int>(der.size()));
    GuestPkcs12* out = nullptr;
    GuestPkcs12* p12 = d2i_PKCS12_bio_nid_postfix(bio, &out);
    Require(p12 && out == p12 && BIO_ctrl_nid_postfix(bio, 10, 0, nullptr) == 0);
    BIO_free_nid_postfix(bio);
    return p12;
}

static std::string CommonName(GuestX509* x) {
    char buffer[64];
    Require(X509_NAME_get_text_by_NID_nid_postfix(X509_get_subject_name_nid_postfix(x), 13, buffer, sizeof(buffer)) > 0);
    return buffer;
}

static void APS5_VABI FreeCert(void* x) {
    X509_free_nid_postfix(static_cast<GuestX509*>(x));
}

static void TestParse(const char* hex) {
    GuestPkcs12* p12 = Read(hex);
    Require(p12->version && ASN1_INTEGER_get_nid_postfix(p12->version) == 3 && !p12->mac && !p12->authsafes);
    GuestEvpPkey* pkey = nullptr;
    GuestX509* cert = nullptr;
    GuestStack* ca = nullptr;
    Require(PKCS12_parse_nid_postfix(p12, "secret", &pkey, &cert, &ca) == 1);
    Require(pkey && pkey->type == 408 && pkey->references == 1 && cert && cert->references == 1 && CommonName(cert) == "leaf.example.test");
    Require(ca && sk_num_nid_postfix(ca) == 2);
    Require(CommonName(static_cast<GuestX509*>(sk_value_nid_postfix(ca, 0))) == "Test Intermediate");
    Require(CommonName(static_cast<GuestX509*>(sk_value_nid_postfix(ca, 1))) == "Test Root");
    GuestEvpPkey* again = nullptr;
    GuestX509* second = nullptr;
    Require(PKCS12_parse_nid_postfix(p12, "secret", &again, &second, &ca) == 1 && sk_num_nid_postfix(ca) == 4 && second != cert);
    EVP_PKEY_free_nid_postfix(again);
    X509_free_nid_postfix(second);
    Require(PKCS12_parse_nid_postfix(p12, "secret", &again, &second, nullptr) == 1 && again && second);
    EVP_PKEY_free_nid_postfix(again);
    X509_free_nid_postfix(second);
    ERR_clear_error_nid_postfix();
    Require(PKCS12_parse_nid_postfix(p12, "wrong", &again, &second, nullptr) == 0 && !again && !second);
    Require(Errors() == std::vector<std::uint64_t>({Code(35, 118, 113)}));
    Require(PKCS12_parse_nid_postfix(p12, nullptr, &again, &second, nullptr) == 0);
    Require(Throws([&] { PKCS12_parse_nid_postfix(p12, "secret", nullptr, &second, nullptr); }));
    sk_pop_free_nid_postfix(ca, FreeCert);
    X509_free_nid_postfix(cert);
    EVP_PKEY_free_nid_postfix(pkey);
    PKCS12_free_nid_postfix(p12);
    Require(Throws([&] { PKCS12_free_nid_postfix(p12); }));
}

static void TestEmptyPassword() {
    GuestPkcs12* p12 = Read(EmptyHex);
    for (const char* pass : {static_cast<const char*>(nullptr), ""}) {
        GuestEvpPkey* pkey = nullptr;
        GuestX509* cert = nullptr;
        GuestStack* ca = nullptr;
        Require(PKCS12_parse_nid_postfix(p12, pass, &pkey, &cert, &ca) == 1);
        Require(pkey && pkey->type == 6 && cert && CommonName(cert) == "Test Intermediate" && ca && sk_num_nid_postfix(ca) == 0);
        sk_pop_free_nid_postfix(ca, FreeCert);
        X509_free_nid_postfix(cert);
        EVP_PKEY_free_nid_postfix(pkey);
    }
    GuestEvpPkey* pkey = nullptr;
    GuestX509* cert = nullptr;
    Require(PKCS12_parse_nid_postfix(p12, "x", &pkey, &cert, nullptr) == 0);
    PKCS12_free_nid_postfix(p12);
    PKCS12_free_nid_postfix(nullptr);

    p12 = Read(NoMacHex);
    ERR_clear_error_nid_postfix();
    Require(PKCS12_parse_nid_postfix(p12, "secret", &pkey, &cert, nullptr) == 0);
    Require(Errors() == std::vector<std::uint64_t>({Code(35, 126, 108), Code(35, 118, 113)}));
    PKCS12_free_nid_postfix(p12);
}

static void TestReaders(const char* path) {
    auto der = Hex(ModernHex);
    FileStream* stream = fopen_nid_postfix(path, "wb");
    Require(stream && fwrite_nid_postfix(der.data(), 1, der.size(), stream) == der.size() && fwrite_nid_postfix("tail", 1, 4, stream) == 4);
    Require(fclose_nid_postfix(stream) == 0);
    stream = fopen_nid_postfix(path, "rb");
    GuestPkcs12* p12 = d2i_PKCS12_fp_nid_postfix(stream, nullptr);
    Require(p12 && d2i_PKCS12_fp_nid_postfix(stream, nullptr) == nullptr);
    Require(fclose_nid_postfix(stream) == 0);
    std::remove(path);
    GuestPkcs12* existing = p12;
    char junk[] = "\x30\x03\x02\x01\x03";
    GuestBio* bio = BIO_new_mem_buf_nid_postfix(junk, 5);
    Require(d2i_PKCS12_bio_nid_postfix(bio, nullptr) == nullptr && BIO_ctrl_nid_postfix(bio, 10, 0, nullptr) == 5);
    Require(Throws([&] { d2i_PKCS12_bio_nid_postfix(bio, &existing); }));
    BIO_free_nid_postfix(bio);
    PKCS12_free_nid_postfix(p12);
    PKCS12_PBE_add_nid_postfix();
}

int main(int argc, char** argv) {
    Require(argc == 2);
    void* const heapApi[10] {};
    ApplicationHeapRegister_nid_no_patch(heapApi);
    TestParse(ModernHex);
    TestParse(LegacyHex);
    TestEmptyPassword();
    TestReaders(argv[1]);
    return 0;
}

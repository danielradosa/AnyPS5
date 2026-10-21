#include "prx/libSceLibreSsl/include/Asn1.hpp"
#include "prx/libSceLibreSsl/include/Internal.hpp"
#include "prx/libSceLibreSsl/src/ObjectTable.hpp"
#include "prx/libc/include/General.hpp"

#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace LibreSsl {

namespace {

constexpr int ErrLibObj = 8;
constexpr int ObjFunctionNid2sn = 104;
constexpr int ObjReasonUnknownNid = 101;
constexpr int ObjectFlagDynamic = 0x01 | 0x04 | 0x08;

struct Table {
    std::vector<std::vector<unsigned char>> encodings;
    std::vector<Asn1Object> objects;
    std::unordered_map<int, std::size_t> byNid;
    std::map<std::string, std::size_t, std::less<>> bySn;
    std::map<std::string, std::size_t, std::less<>> byLn;
    std::map<std::vector<unsigned char>, std::size_t> byOid;
    int count = 0;

    explicit Table(bool version3) {
        std::map<int, ObjectEntry> entries;
        for (const auto& entry : ObjectTable26) entries[entry.nid] = entry;
        count = ObjectTable26Count;
        if (version3) {
            for (const auto& entry : ObjectTable34Changes) entries[entry.nid] = entry;
            count = ObjectTable34Count;
        }
        encodings.reserve(entries.size());
        objects.reserve(entries.size());
        for (const auto& [nid, entry] : entries) {
            encodings.push_back(entry.oid ? EncodeObjectIdentifier(entry.oid) : std::vector<unsigned char>{});
            const auto& encoding = encodings.back();
            objects.push_back({entry.sn, entry.ln, nid, static_cast<int>(encoding.size()), encoding.empty() ? nullptr : encoding.data(), 0});
            const std::size_t index = objects.size() - 1;
            byNid.emplace(nid, index);
            if (entry.sn) bySn.emplace(entry.sn, index);
            if (entry.ln) byLn.emplace(entry.ln, index);
            if (!encoding.empty()) byOid.emplace(encoding, index);
        }
    }
};

const Table& Builtin(bool version3) {
    static const Table v2(false);
    static const Table v3(true);
    return version3 ? v3 : v2;
}

struct Added {
    std::string sn;
    std::string ln;
    std::vector<unsigned char> encoding;
    Asn1Object object;
};

struct AddedTable {
    std::mutex mutex;
    std::vector<std::unique_ptr<Added>> objects;
    int nextNid = ObjectTable26Count;
    bool removed = false;
};

AddedTable& AddedObjects() {
    static AddedTable table;
    return table;
}

struct CreatedTable {
    std::mutex mutex;
    std::unordered_set<const Asn1Object*> objects;
};

CreatedTable& CreatedObjects() {
    static CreatedTable table;
    return table;
}

const Asn1Object* BuiltinByOid(const Table& table, const unsigned char* data, std::size_t length) {
    if (!data || !length) return nullptr;
    const auto found = table.byOid.find(std::vector<unsigned char>(data, data + length));
    return found == table.byOid.end() ? nullptr : &table.objects[found->second];
}

const Asn1Object* AddedByOid(const unsigned char* data, std::size_t length) {
    for (const auto& added : AddedObjects().objects) {
        if (added->encoding.size() == length && std::memcmp(added->encoding.data(), data, length) == 0) return &added->object;
    }
    return nullptr;
}

bool ParseArc(std::string_view text, std::uint64_t& value) {
    if (text.empty()) return false;
    value = 0;
    for (const char character : text) {
        if (character < '0' || character > '9') return false;
    }
    for (const char character : text) {
        const auto digit = static_cast<std::uint64_t>(character - '0');
        if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10) throw std::invalid_argument("OBJ: OID arcs above 64 bits are not supported");
        value = value * 10 + digit;
    }
    return true;
}
}

std::vector<unsigned char> EncodeObjectIdentifier(const char* dotted) {
    std::vector<std::uint64_t> arcs;
    const std::string_view text(dotted);
    std::size_t start = 0;
    while (true) {
        const auto end = text.find('.', start);
        const auto part = text.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start);
        std::uint64_t value = 0;
        if (!ParseArc(part, value)) return {};
        if (part.size() > 1 && part[0] == '0') throw std::invalid_argument("OBJ: an OID arc with a leading zero is handled differently between LibreSSL versions");
        arcs.push_back(value);
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    if (arcs.size() < 2 || arcs[0] > 2 || (arcs[0] < 2 && arcs[1] >= 40)) return {};
    if (arcs[1] > std::numeric_limits<std::uint64_t>::max() - 80) throw std::invalid_argument("OBJ: OID arcs above 64 bits are not supported");
    std::vector<std::uint64_t> encoded{arcs[0] * 40 + arcs[1]};
    encoded.insert(encoded.end(), arcs.begin() + 2, arcs.end());
    std::vector<unsigned char> bytes;
    for (auto arc : encoded) {
        unsigned char groups[10];
        int count = 0;
        do {
            groups[count++] = static_cast<unsigned char>(arc & 0x7f);
            arc >>= 7;
        } while (arc);
        while (count--) bytes.push_back(static_cast<unsigned char>(groups[count] | (count ? 0x80 : 0)));
    }
    return bytes;
}

bool DecodeObjectIdentifier(const unsigned char* data, std::size_t length, std::string& out) {
    out.clear();
    if (!data || !length) return false;
    std::uint64_t value = 0;
    bool first = true;
    bool pending = false;
    for (std::size_t index = 0; index < length; ++index) {
        const unsigned char byte = data[index];
        if (!pending && byte == 0x80) return false;
        if (value > (std::numeric_limits<std::uint64_t>::max() >> 7)) throw std::invalid_argument("OBJ: OID arcs above 64 bits are not supported");
        value = value << 7 | (byte & 0x7f);
        pending = (byte & 0x80) != 0;
        if (pending) continue;
        if (first) {
            const std::uint64_t top = value < 40 ? 0 : value < 80 ? 1 : 2;
            out += std::to_string(top) + '.' + std::to_string(value - top * 40);
            first = false;
        } else {
            out += '.' + std::to_string(value);
        }
        value = 0;
    }
    return !pending;
}

const Asn1Object* TableObject(int nid, bool version3) {
    const auto& table = Builtin(version3);
    const auto found = table.byNid.find(nid);
    if (found != table.byNid.end()) return &table.objects[found->second];
    if (version3) return nullptr;
    auto& added = AddedObjects();
    std::lock_guard lock(added.mutex);
    for (const auto& object : added.objects) {
        if (object->object.nid == nid) return &object->object;
    }
    return nullptr;
}

int ObjectNid(const Asn1Object* object, bool version3) {
    if (!object) return NidUndef;
    if (object->nid != NidUndef) return object->nid;
    if (!object->data || object->length <= 0) return NidUndef;
    if (const auto* found = BuiltinByOid(Builtin(version3), object->data, static_cast<std::size_t>(object->length))) return found->nid;
    if (version3) return NidUndef;
    auto& added = AddedObjects();
    std::lock_guard lock(added.mutex);
    const auto* found = AddedByOid(object->data, static_cast<std::size_t>(object->length));
    return found ? found->nid : NidUndef;
}

const char* TableShortName(int nid, const char* function) {
    if (nid != NidUndef) {
        const auto& table = Builtin(false);
        const auto found = table.byNid.find(nid);
        if (found != table.byNid.end() && !table.objects[found->second].sn)
            throw std::invalid_argument(std::string(function) + ": NID " + std::to_string(nid) + " has no short name in the object table, which LibreSSL resolves in a way its manuals do not document");
    }
    const auto* object = TableObject(nid, false);
    return object ? object->sn : nullptr;
}

bool ObjectText(const Asn1Object* object, bool noName, std::string& out, const char* function) {
    out.clear();
    if (!object || !object->data || object->length <= 0) return false;
    if (!noName) {
        if (object->ln || object->sn) {
            out = object->ln ? object->ln : object->sn;
            return true;
        }
        if (ObjectNid(object, false) != NidUndef)
            throw std::invalid_argument(std::string(function) + ": the object has no names but its OID is in the object table; which text 2.6 prints for it is not documented");
    }
    return DecodeObjectIdentifier(object->data, static_cast<std::size_t>(object->length), out);
}

bool ReleaseCreatedObject(Asn1Object* object) {
    auto& created = CreatedObjects();
    {
        std::lock_guard lock(created.mutex);
        if (!created.objects.erase(object)) return false;
    }
    delete[] object->data;
    delete object;
    return true;
}

void ObjectsCleanup() {
    auto& added = AddedObjects();
    std::lock_guard lock(added.mutex);
    if (!added.objects.empty()) added.removed = true;
    added.objects.clear();
}

}

using namespace LibreSsl;

extern "C" {
int APS5_VABI BIO_write_nid_postfix(Bio* b, const void* data, int len);

const char* APS5_VABI OBJ_nid2sn_nid_postfix(int n) {
    const char* name = TableShortName(n, __func__);
    if (!name && !TableObject(n, false)) PushError(ErrLibObj, ObjFunctionNid2sn, ObjReasonUnknownNid);
    return name;
}

int APS5_VABI OBJ_sn2nid_nid_postfix(const char* s) {
    if (!s) throw std::invalid_argument("OBJ_sn2nid: null name");
    const auto& table = Builtin(false);
    const auto found = table.bySn.find(std::string_view(s));
    if (found != table.bySn.end()) return table.objects[found->second].nid;
    {
        auto& added = AddedObjects();
        std::lock_guard lock(added.mutex);
        for (const auto& object : added.objects) {
            if (object->sn == s) return object->object.nid;
        }
    }
    const auto byLong = table.byLn.find(std::string_view(s));
    if (byLong != table.byLn.end() && !table.objects[byLong->second].sn)
        throw std::invalid_argument(std::string("OBJ_sn2nid: '") + s + "' is the long name of an object without a short name, which LibreSSL resolves in a way its manuals do not document");
    return NidUndef;
}

int APS5_VABI OBJ_obj2nid_nid_postfix(const Asn1Object* o) {
    return ObjectNid(o, false);
}

int APS5_VABI OBJ_create_nid_postfix(const char* oid, const char* sn, const char* ln) {
    if (!oid) throw std::invalid_argument("OBJ_create: null OID");
    if (!sn || !ln) throw std::invalid_argument("OBJ_create: an object without a short or a long name is not supported");
    auto encoding = EncodeObjectIdentifier(oid);
    if (encoding.empty()) return NidUndef;
    const auto& table = Builtin(false);
    if (table.bySn.count(std::string_view(sn)) || table.byLn.count(std::string_view(ln)) || table.byOid.count(encoding) ||
        table.bySn.count(std::string_view(ln)) || table.byLn.count(std::string_view(sn)))
        throw std::invalid_argument("OBJ_create: an object whose OID or names are already in the table is not supported, the entry a later lookup finds is not documented");
    auto& added = AddedObjects();
    std::lock_guard lock(added.mutex);
    if (added.removed) throw std::logic_error("OBJ_create: whether NIDs are handed out again after OBJ_cleanup removed objects is not documented");
    for (const auto& object : added.objects) {
        if (object->encoding == encoding || object->sn == sn || object->ln == ln || object->sn == ln || object->ln == sn)
            throw std::invalid_argument("OBJ_create: an object whose OID or names were already added is not supported, the entry a later lookup finds is not documented");
    }
    auto created = std::make_unique<Added>(Added{sn, ln, std::move(encoding), {}});
    created->object = {created->sn.c_str(), created->ln.c_str(), added.nextNid, static_cast<int>(created->encoding.size()), created->encoding.data(), 0};
    added.objects.push_back(std::move(created));
    return added.nextNid++;
}

int APS5_VABI i2a_ASN1_OBJECT_nid_postfix(Bio* bp, const Asn1Object* a) {
    if (!bp) throw std::invalid_argument("i2a_ASN1_OBJECT: null BIO");
    std::string text;
    if (!a || !a->data || a->length <= 0) text = "NULL";
    else if (!ObjectText(a, false, text, __func__)) text = "<INVALID>";
    return BIO_write_nid_postfix(bp, text.data(), static_cast<int>(text.size()));
}

int LibreSslObjCmp_nid_no_patch(const Asn1Object* a, const Asn1Object* b) {
    if (!a || !b) throw std::invalid_argument("OBJ_cmp: null object");
    if (a->length != b->length) return a->length - b->length;
    if (a->length <= 0) return 0;
    return std::memcmp(a->data, b->data, static_cast<std::size_t>(a->length));
}

Asn1Object* LibreSslObjTxt2Obj_nid_no_patch(const char* s, int noName) {
    if (!s) throw std::invalid_argument("OBJ_txt2obj: null text");
    if (!noName) {
        const auto& table = Builtin(true);
        const auto bySn = table.bySn.find(std::string_view(s));
        const auto byLn = table.byLn.find(std::string_view(s));
        if (bySn != table.bySn.end() && byLn != table.byLn.end() && bySn->second != byLn->second)
            throw std::invalid_argument(std::string("OBJ_txt2obj: '") + s + "' is the short name of one object and the long name of another; which one LibreSSL returns is not documented");
        if (bySn != table.bySn.end()) return const_cast<Asn1Object*>(&table.objects[bySn->second]);
        if (byLn != table.byLn.end()) return const_cast<Asn1Object*>(&table.objects[byLn->second]);
    }
    const auto encoding = EncodeObjectIdentifier(s);
    if (encoding.empty()) return nullptr;
    auto data = std::make_unique<unsigned char[]>(encoding.size());
    std::memcpy(data.get(), encoding.data(), encoding.size());
    auto* object = new Asn1Object{nullptr, nullptr, NidUndef, static_cast<int>(encoding.size()), data.release(), ObjectFlagDynamic};
    auto& created = CreatedObjects();
    std::lock_guard lock(created.mutex);
    created.objects.insert(object);
    return object;
}

}

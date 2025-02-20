import yaml
import hashlib
import struct
from collections import OrderedDict
import time
import copy

class Sharemap:

    # pylint: disable=use-dict-literal
    SCHEMA_TYPES = dict(
        # tuple of size in bytes, c++ type, python struct format
        u8=(1, "std::uint8_t", 'B'),
        u16=(2, "std::uint16_t", 'H'),
        u32=(4, "std::uint32_t", 'I'),
        u64=(8, "std::uint64_t", 'Q'),
        i8=(1, "std::int8_t", 'b'),
        i16=(2, "std::int16_t", 'h'),
        i32=(4, "std::int32_t", 'i'),
        i64=(8, "std::int64_t", 'q'),
        f32=(4, "float", '4s'),
        f64=(8, "double", '8s'),
        boolean=(1, "bool", '?'),
        string=(64, "std::array<char, STRING_BUFFER_SIZE>", '64s'),
    )

    def __init__(self, schema):

        # create hash
        schema_hash = hashlib.sha256()
        for name, details in schema.items():
            schema_hash.update(name.encode())
            schema_hash.update(details["type"].encode())
        self._hash = int(schema_hash.hexdigest()[:16], 16)

        # load shared fields
        self._fields = []
        self._fields.append(
            dict(
                name="source_id",
                desc="id of where the data comes from",
                type="u16",
                default="",
            )
        )
        self._fields.append(
            dict(
                name="schema_hash",
                desc="hash of the schema used to ensure compatibility",
                type="u64",
                default="HASH",
            )
        )
        self._fields.append(
            dict(
                name="unix_timestamp_ns",
                desc="timestamp that counts the amount of time (in nanoseconds) since the unix epoch",
                type="i64",
                default="time_ns_since_epoch()",
            )
        )

        for name, details in schema.items():
            self._fields.append(
                dict(
                    name=name,
                    desc=details["desc"],
                    type=details["type"],
                    default="",
                )
            )

        self._struct_format = '!' #network endian
        self._struct_format += ''.join([Sharemap.SCHEMA_TYPES[f['type']][2] for f in self._fields])
        self._packed_size = sum([Sharemap.SCHEMA_TYPES[f['type']][0] for f in self._fields])
        #print(self._struct_format)
        #print(hex(self._hash))

    def get_hash(self): return self._hash

    def get_fields(self): return self._fields

    def unpack(self, buff):
        """
        Unpack a sharemap buffer into a dictionary of key/values
        """
        if len(buff) != self._packed_size:
            raise Exception(f"incompatible sharemap: expected size {self._packed_size} bytes, but got size {len(buff)} bytes")
        out = OrderedDict()
        values = struct.unpack(self._struct_format, buff)
        for i, field in enumerate(self._fields):
            v = values[i]
            # unpack floats as little endian
            if field['type'] == 'f32': v = struct.unpack('=f', v)[0]
            if field['type'] == 'f64': v = struct.unpack('=d', v)[0]
            # convert bytes to string and strip null termination characters
            if isinstance(v, bytes): v = v.decode('utf-8').rstrip('\x00')
            out[field['name']] = v
        if out['schema_hash'] != self._hash:
            raise Exception(f"incompatible sharemap: expected hash {hex(self._hash)}, but got hash {hex(out['schema_hash'])}")
        return out

    def pack(self, config):
        """
        Pack a config dictionary into a sharemap buffer
        """
        config = dict(copy.copy(config))
        config['schema_hash'] = self._hash
        config['unix_timestamp_ns'] = time.time_ns()
        config['source_id'] = 0
        args = list()
        known_keys = set()
        for i, field in enumerate(self._fields):
            fname = field['name']
            known_keys.add(fname)
            v = config.get(fname)
            if v is None: raise Exception(f"sharemap missing field {fname}")
            # convert string to the bytes type
            if isinstance(v, str): v = v.encode()
            # pack floats into little endian
            if field['type'] == 'f32': v = struct.pack('=f', float(v))
            if field['type'] == 'f64': v = struct.pack('=d', float(v))
            args.append(v)
        unknown_keys = config.keys() - known_keys
        if unknown_keys: raise Exception(f"sharemap unreconginized fields {unknown_keys}")
        return struct.pack(self._struct_format, *args)

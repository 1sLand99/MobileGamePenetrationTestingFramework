struct MonoMethod;
struct MonoClass;
struct MonoJitInfo;
struct StackFrameInfo;
struct MonoJitExceptionInfo;
struct MonoClassField;
struct MonoType;
struct MonoCustomMod;

#define MONO_ZERO_LEN_ARRAY 1

typedef enum {
    MONO_TYPE_END        = 0x00,       /* End of List */
    MONO_TYPE_VOID       = 0x01,
    MONO_TYPE_BOOLEAN    = 0x02,
    MONO_TYPE_CHAR       = 0x03,
    MONO_TYPE_I1         = 0x04,
    MONO_TYPE_U1         = 0x05,
    MONO_TYPE_I2         = 0x06,
    MONO_TYPE_U2         = 0x07,
    MONO_TYPE_I4         = 0x08,
    MONO_TYPE_U4         = 0x09,
    MONO_TYPE_I8         = 0x0a,
    MONO_TYPE_U8         = 0x0b,
    MONO_TYPE_R4         = 0x0c,
    MONO_TYPE_R8         = 0x0d,
    MONO_TYPE_STRING     = 0x0e,
    MONO_TYPE_PTR        = 0x0f,       /* arg: <type> token */
    MONO_TYPE_BYREF      = 0x10,       /* arg: <type> token */
    MONO_TYPE_VALUETYPE  = 0x11,       /* arg: <type> token */
    MONO_TYPE_CLASS      = 0x12,       /* arg: <type> token */
    MONO_TYPE_VAR	     = 0x13,	   /* number */
    MONO_TYPE_ARRAY      = 0x14,       /* type, rank, boundsCount, bound1, loCount, lo1 */
    MONO_TYPE_GENERICINST= 0x15,	   /* <type> <type-arg-count> <type-1> \x{2026} <type-n> */
    MONO_TYPE_TYPEDBYREF = 0x16,
    MONO_TYPE_I          = 0x18,
    MONO_TYPE_U          = 0x19,
    MONO_TYPE_FNPTR      = 0x1b,	      /* arg: full method signature */
    MONO_TYPE_OBJECT     = 0x1c,
    MONO_TYPE_SZARRAY    = 0x1d,       /* 0-based one-dim-array */
    MONO_TYPE_MVAR	     = 0x1e,       /* number */
    MONO_TYPE_CMOD_REQD  = 0x1f,       /* arg: typedef or typeref token */
    MONO_TYPE_CMOD_OPT   = 0x20,       /* optional arg: typedef or typref token */
    MONO_TYPE_INTERNAL   = 0x21,       /* CLR internal type */

    MONO_TYPE_MODIFIER   = 0x40,       /* Or with the following types */
    MONO_TYPE_SENTINEL   = 0x41,       /* Sentinel for varargs method signature */
    MONO_TYPE_PINNED     = 0x45,       /* Local var that points to pinned object */

    MONO_TYPE_ENUM       = 0x55        /* an enumeration */
} MonoTypeEnum;



typedef enum {
    FRAME_TYPE_MANAGED = 0,
    FRAME_TYPE_DEBUGGER_INVOKE = 1,
    FRAME_TYPE_MANAGED_TO_NATIVE = 2,
    FRAME_TYPE_TRAMPOLINE = 3,
    FRAME_TYPE_INTERP = 4,
    FRAME_TYPE_INTERP_TO_MANAGED = 5,
    FRAME_TYPE_NUM = 6
} MonoStackFrameType;

typedef enum {
    MONO_UNWIND_NONE = 0x0,
    MONO_UNWIND_LOOKUP_IL_OFFSET = 0x1,
    MONO_UNWIND_LOOKUP_ACTUAL_METHOD = 0x2,
    MONO_UNWIND_REG_LOCATIONS = 0x4,
    MONO_UNWIND_DEFAULT = MONO_UNWIND_LOOKUP_ACTUAL_METHOD,
    MONO_UNWIND_SIGNAL_SAFE = MONO_UNWIND_NONE,
    MONO_UNWIND_LOOKUP_ALL = MONO_UNWIND_LOOKUP_IL_OFFSET | MONO_UNWIND_LOOKUP_ACTUAL_METHOD,
} MonoUnwindOptions;

struct MonoCustomMod{
    unsigned int required : 1;
    unsigned int token    : 31;
};

struct MonoType {
    union {
        MonoClass *klass; /* for VALUETYPE and CLASS */
        MonoType *type;   /* for PTR */
        void *array; /* for ARRAY */
        void *method;
        void *generic_param; /* for VAR and MVAR */
        void *generic_class; /* for GENERICINST */
    } data;
    unsigned int attrs    : 16; /* param attributes or field flags */
    MonoTypeEnum type     : 8;
    unsigned int num_mods : 6;  /* max 64 modifiers follow at the end */
    unsigned int byref    : 1;
    unsigned int pinned   : 1;  /* valid when included in a local var signature */
    MonoCustomMod modifiers [MONO_ZERO_LEN_ARRAY]; /* this may grow */
};

struct MonoClassField {
    MonoType        *type;
    const char      *name;
    MonoClass       *parent;
    int              offset;
};

struct StackFrameInfo{
    MonoStackFrameType type;
    MonoJitInfo *ji;
    MonoMethod *method;
    MonoMethod *actual_method;
    void  *domain;
    bool managed;
    bool async_context;
    int native_offset;
    int il_offset;
    void * interp_exit_data;
    void * interp_frame;
    void * lmf;
    uint32_t unwind_info_len;
    uint8_t *unwind_info;
    uint64_t **reg_locations;
};

struct MonoJitExceptionInfo{
    uint32_t  flags;
    int32_t   exvar_offset;
    void * try_start;
    void * try_end;
    void * handler_start;
    int clause_index;
    uint32_t try_offset;
    uint32_t try_len;
    uint32_t handler_offset;
    uint32_t handler_len;
    union {
        MonoClass *catch_class;
        void * filter;
        void * handler_end;
    } data;
};

struct MonoJitInfo {
    union {
        MonoMethod *method;
        void *image;
        void * aot_info;
        void * tramp_info;
    } d;
    union {
        struct _MonoJitInfo *next_jit_code_hash;
        struct _MonoJitInfo *next_tombstone;
    } n;
    void *    code_start;
    uint32_t     unwind_info;
    int         code_size;
    uint32_t     num_clauses:15;
    /* Whenever the code is domain neutral or 'shared' */
    bool    domain_neutral:1;
    bool    has_generic_jit_info:1;
    bool    has_try_block_holes:1;
    bool    has_arch_eh_info:1;
    bool    has_thunk_info:1;
    bool    has_unwind_info:1;
    bool    from_aot:1;
    bool    from_llvm:1;
    bool    dbg_attrs_inited:1;
    bool    dbg_hidden:1;
    bool    async:1;
    bool    dbg_step_through:1;
    bool    dbg_non_user_code:1;
    bool    is_trampoline:1;
    bool    is_interp:1;
    void *    gc_info; /* Currently only used by SGen */
    MonoJitExceptionInfo clauses [MONO_ZERO_LEN_ARRAY];
};

struct MonoClass {
    MonoClass *element_class;
    MonoClass *cast_class;
    MonoClass **supertypes;
    uint16_t     idepth;
    uint8_t     rank;
    int        instance_size; /* object instance size */
    uint_t inited          : 1;
    uint_t size_inited     : 1;
    uint_t valuetype       : 1; /* derives from System.ValueType */
    uint_t enumtype        : 1; /* derives from System.Enum */
    uint_t blittable       : 1; /* class is blittable */
    uint_t unicode         : 1; /* class uses unicode char when marshalled */
    uint_t wastypebuilder  : 1; /* class was created at runtime from a TypeBuilder */
    uint_t is_array_special_interface : 1; /* gtd or ginst of once of the magic interfaces that arrays implement */
    uint8_t min_align;
    uint_t packing_size    : 4;
    uint_t ghcimpl         : 1; /* class has its own GetHashCode impl */
    uint_t has_finalize    : 1; /* class has its own Finalize impl */
//#ifndef DISABLE_REMOTING
    uint_t marshalbyref    : 1; /* class is a MarshalByRefObject */
    uint_t contextbound    : 1; /* class is a ContextBoundObject */
//#endif
    /* next byte */
    uint_t delegate        : 1; /* class is a Delegate */
    uint_t gc_descr_inited : 1; /* gc_descr is initialized */
    uint_t has_cctor       : 1; /* class has a cctor */
    uint_t has_references  : 1; /* it has GC-tracked references in the instance */
    uint_t has_static_refs : 1; /* it has static fields that are GC-tracked */
    uint_t no_special_static_fields : 1; /* has no thread/context static fields */
    uint_t is_com_object : 1;
    uint_t nested_classes_inited : 1; /* Whenever nested_class is initialized */
    uint_t class_kind : 3; /* One of the values from MonoTypeKind */
    uint_t interfaces_inited : 1; /* interfaces is initialized */
    uint_t simd_type : 1; /* class is a simd intrinsic type */
    uint_t has_finalize_inited    : 1; /* has_finalize is initialized */
    uint_t fields_inited : 1; /* setup_fields () has finished */
    uint_t has_failure : 1; /* See mono_class_get_exception_data () for a MonoErrorBoxed with the details */
    uint_t has_weak_fields : 1; /* class has weak reference fields */
    MonoClass  *parent;
    MonoClass  *nested_in;
    void *image;
    const char *name;
    const char *name_space;
    uint32_t    type_token;
    int        vtable_size; /* number of slots */
    uint16_t     interface_count;
    uint32_t     interface_id;        /* unique inderface id (for interfaces) */
    uint32_t     max_interface_id;
    uint16_t     interface_offsets_count;
    MonoClass **interfaces_packed;
    uint16_t    *interface_offsets_packed;
/* enabled only with small config for now: we might want to do it unconditionally */
//#ifdef MONO_SMALL_CONFIG
#define COMPRESSED_INTERFACE_BITMAP 1
//#endif
    uint8_t     *interface_bitmap;
    MonoClass **interfaces;
    union {
        int class_size; /* size of area for static fields */
        int element_size; /* for array types */
        int generic_param_token; /* for generic param types, both var and mvar */
    } sizes;
    MonoClassField *fields;
    MonoMethod **methods;
    MonoType this_arg;
    MonoType byval_arg;
    void * gc_descr;
    void *runtime_info;
    MonoMethod **vtable;
    void * infrequent_data;
    void *unity_user_data;
};

struct MonoMethod {
    uint16_t flags;  /* method flags */
    uint16_t iflags; /* method implementation flags */
    uint32_t token;
    MonoClass *klass; /* To what class does this method belong */
    void  *signature;
    const char *name;
    unsigned int inline_info:1;
    unsigned int inline_failure:1;
    unsigned int wrapper_type:5;
    unsigned int string_ctor:1;
    unsigned int save_lmf:1;
    unsigned int dynamic:1; /* created & destroyed during runtime */
    unsigned int sre_method:1; /* created at runtime using Reflection.Emit */
    unsigned int is_generic:1; /* whenever this is a generic method definition */
    unsigned int is_inflated:1; /* whether we're a MonoMethodInflated */
    unsigned int skip_visibility:1; /* whenever to skip JIT visibility checks */
    unsigned int verification_success:1; /* whether this method has been verified successfully.*/
    signed int slot : 16;
};
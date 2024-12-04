typedef struct _MonoJitInfo MonoJitInfo;
typedef struct MonoMethod;
typedef struct MonoClass;
struct MonoClass {
    /* element class for arrays and enum basetype for enums */
    MonoClass *element_class;
    /* used for subtype checks */
    MonoClass *cast_class;

    /* for fast subtype checks */
    MonoClass **supertypes;
    uint16_t    idepth;

    /* array dimension */
    uint8_t    rank;

    int        instance_size; /* object instance size */

    uint inited          : 1;

    /* A class contains static and non static data. Static data can be
     * of the same type as the class itselfs, but it does not influence
     * the instance size of the class. To avoid cyclic calls to
     * mono_class_init (from mono_class_instance_size ()) we first
     * initialise all non static fields. After that we set size_inited
     * to 1, because we know the instance size now. After that we
     * initialise all static fields.
     */

    /* ALL BITFIELDS SHOULD BE WRITTEN WHILE HOLDING THE LOADER LOCK */
    uint size_inited     : 1;
    uint valuetype       : 1; /* derives from System.ValueType */
    uint enumtype        : 1; /* derives from System.Enum */
    uint blittable       : 1; /* class is blittable */
    uint unicode         : 1; /* class uses unicode char when marshalled */
    uint wastypebuilder  : 1; /* class was created at runtime from a TypeBuilder */
    uint is_array_special_interface : 1; /* gtd or ginst of once of the magic interfaces that arrays implement */

    /* next byte */
    uint8_t min_align;

    /* next byte */
    uint packing_size    : 4;
    uint ghcimpl         : 1; /* class has its own GetHashCode impl */
    uint has_finalize    : 1; /* class has its own Finalize impl */
#ifndef DISABLE_REMOTING
    uint marshalbyref    : 1; /* class is a MarshalByRefObject */
    uint contextbound    : 1; /* class is a ContextBoundObject */
#endif
    /* next byte */
    uint delegate        : 1; /* class is a Delegate */
    uint gc_descr_inited : 1; /* gc_descr is initialized */
    uint has_cctor       : 1; /* class has a cctor */
    uint has_references  : 1; /* it has GC-tracked references in the instance */
    uint has_static_refs : 1; /* it has static fields that are GC-tracked */
    uint no_special_static_fields : 1; /* has no thread/context static fields */
    /* directly or indirectly derives from ComImport attributed class.
     * this means we need to create a proxy for instances of this class
     * for COM Interop. set this flag on loading so all we need is a quick check
     * during object creation rather than having to traverse supertypes
     */
    uint is_com_object : 1;
    uint nested_classes_inited : 1; /* Whenever nested_class is initialized */

    /* next byte*/
    uint class_kind : 3; /* One of the values from MonoTypeKind */
    uint interfaces_inited : 1; /* interfaces is initialized */
    uint simd_type : 1; /* class is a simd intrinsic type */
    uint has_finalize_inited    : 1; /* has_finalize is initialized */
    uint fields_inited : 1; /* setup_fields () has finished */
    uint has_failure : 1; /* See mono_class_get_exception_data () for a MonoErrorBoxed with the details */
    uint has_weak_fields : 1; /* class has weak reference fields */

    MonoClass  *parent;
    MonoClass  *nested_in;

    void * image;
    const char *name;
    const char *name_space;

    uint32_t    type_token;
    int        vtable_size; /* number of slots */

    uint16_t     interface_count;
    uint32_t     interface_id;        /* unique inderface id (for interfaces) */
    uint32_t     max_interface_id;

    uint16_t     interface_offsets_count;
    MonoClass **interfaces_packed;
    uint16_t   *interface_offsets_packed;
/* enabled only with small config for now: we might want to do it unconditionally */
#ifdef MONO_SMALL_CONFIG
#define COMPRESSED_INTERFACE_BITMAP 1
#endif
    uint8_t    *interface_bitmap;

    MonoClass **interfaces;

    union {
        int class_size; /* size of area for static fields */
        int element_size; /* for array types */
        int generic_param_token; /* for generic param types, both var and mvar */
    } sizes;

    /*
     * Field information: Type and location from object base
     */
    void * fields;

    MonoMethod **methods;

    /* used as the type of the this argument and when passing the arg by value */
    void * this_arg;
    void * byval_arg;

    void * gc_descr;

    void *runtime_info;

    /* Generic vtable. Initialized by a call to mono_class_setup_vtable () */
    MonoMethod **vtable;

    /* Infrequently used items. See class-accessors.c: InfrequentDataKind for what goes into here. */
    void * infrequent_data;

    void * unity_user_data;
};

typedef enum {
    /* Normal managed frames */
    FRAME_TYPE_MANAGED = 0,
    /* Pseudo frame marking the start of a method invocation done by the soft debugger */
    FRAME_TYPE_DEBUGGER_INVOKE = 1,
    /* Frame for transitioning to native code */
    FRAME_TYPE_MANAGED_TO_NATIVE = 2,
    FRAME_TYPE_TRAMPOLINE = 3,
    /* Interpreter frame */
    FRAME_TYPE_INTERP = 4,
    /* Frame for transitioning from interpreter to managed code */
    FRAME_TYPE_INTERP_TO_MANAGED = 5,
    FRAME_TYPE_NUM = 6
} MonoStackFrameType;

typedef enum {
    MONO_UNWIND_NONE = 0x0,
    MONO_UNWIND_LOOKUP_IL_OFFSET = 0x1,
    /* NOT signal safe */
    MONO_UNWIND_LOOKUP_ACTUAL_METHOD = 0x2,
    /*
     * Store the locations where caller-saved registers are saved on the stack in
     * frame->reg_locations. The pointer is only valid during the call to the unwind
     * callback.
     */
    MONO_UNWIND_REG_LOCATIONS = 0x4,
    MONO_UNWIND_DEFAULT = MONO_UNWIND_LOOKUP_ACTUAL_METHOD,
    MONO_UNWIND_SIGNAL_SAFE = MONO_UNWIND_NONE,
    MONO_UNWIND_LOOKUP_ALL = MONO_UNWIND_LOOKUP_IL_OFFSET | MONO_UNWIND_LOOKUP_ACTUAL_METHOD,
} MonoUnwindOptions;

typedef struct {
    MonoStackFrameType type;
    /*
     * For FRAME_TYPE_MANAGED, otherwise NULL.
     */
    MonoJitInfo *ji;
    /*
     * Same as ji->method.
     * Not valid if ASYNC_CONTEXT is true.
     */
    MonoMethod *method;
    /*
     * If ji->method is a gshared method, this is the actual method instance.
     * This is only filled if lookup for actual method was requested (MONO_UNWIND_LOOKUP_ACTUAL_METHOD)
     * Not valid if ASYNC_CONTEXT is true.
     */
    MonoMethod *actual_method;
    /* The domain containing the code executed by this frame */
    void *domain;
    /* Whenever method is a user level method */
    bool managed;
    /*
     * Whenever this frame was loaded in async context.
     */
    bool async_context;
    int native_offset;
    /*
     * IL offset of this frame.
     * Only available if the runtime have debugging enabled (--debug switch) and
     *  il offset resultion was requested (MONO_UNWIND_LOOKUP_IL_OFFSET)
     */
    int il_offset;

    /* For FRAME_TYPE_INTERP_EXIT */
    void * interp_exit_data;

    /* For FRAME_TYPE_INTERP */
    void * interp_frame;

    /* The next fields are only useful for the jit */
    void * lmf;
    uint32_t unwind_info_len;
    uint8_t * unwind_info;

    void **reg_locations;
} MonoStackFrameInfo;

/*Index into MonoThreadState::unwind_data. */
enum {
    MONO_UNWIND_DATA_DOMAIN,
    MONO_UNWIND_DATA_LMF,
    MONO_UNWIND_DATA_JIT_TLS,
};

struct _MonoJitInfo {
    /* NOTE: These first two elements (method and
       next_jit_code_hash) must be in the same order and at the
       same offset as in RuntimeMethod, because of the jit_code_hash
       internal hash table in MonoDomain. */
    union {
        MonoMethod * method;
        void * image;
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
    uint32_t    num_clauses:15;
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
    /* Whenever this jit info was loaded in async context */
    bool    async:1;
    bool    dbg_step_through:1;
    bool    dbg_non_user_code:1;
    /*
     * Whenever this jit info refers to a trampoline.
     * d.tramp_info contains additional data in this case.
     */
    bool    is_trampoline:1;
    /* Whenever this jit info refers to an interpreter method */
    bool    is_interp:1;

    bool    dbg_ignore : 1;

    /* FIXME: Embed this after the structure later*/
    void *    gc_info; /* Currently only used by SGen */

    //MonoJitExceptionInfo clauses [0];
};

typedef struct MonoMethod {
    uint16_t flags;  /* method flags */
    uint16_t iflags; /* method implementation flags */
    uint32_t token;
    MonoClass * mono_class; /* To what class does this method belong */
    void * MonoMethodSignature;
    /* name is useful mostly for debugging */
    const char *name;
#ifdef IL2CPP_ON_MONO
    void* method_pointer;
	void* invoke_pointer;
#endif
    /* this is used by the inlining algorithm */
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

    /*
     * If is_generic is TRUE, the generic_container is stored in image->property_hash,
     * using the key MONO_METHOD_PROP_GENERIC_CONTAINER.
     */
};


typedef MonoStackFrameInfo StackFrameInfo;
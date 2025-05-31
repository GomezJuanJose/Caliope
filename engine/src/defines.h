    #ifdef CE_PLATFORM_WINDOWS
        #ifdef CE_EXPORT_DLL
            #define CE_API __declspec(dllexport)
        #else
            #define CE_API __declspec(dllimport)
        #endif
    #else
        #error Caliope at the moment only supports Windows!
    #endif

    // Inlining
    #ifdef _MSC_VER
        #define CE_INLINE __forinline
        #define CE_NOINLINE __declspec(noinline)
    #else
        #define CE_INLINE static inline
        #define CE_NOINLINE
    #endif

    /** @brief Gets the number of bytes from amount of gibibytes (GiB) (1024 * 1024 * 1024) */
    #define GIBIBYTES(amount) amount * 1024 * 1024 * 1024
    /** @brief Gets the number of bytes from amount of mebibytes (MiB) (1024 * 1024) */
    #define MEBIBYTES(amount) amount * 1024 * 1024
    /** @brief Gets the number of bytes from amount of kibibytes (KiB) (1024) */
    #define KIBIBYTES(amount) amount * 1024

    /** @brief Gets the number of bytes from amount of gigabytes (GiB) (1000 * 1000 * 1000) */
    #define GIGABYTES(amount) amount * 1000 * 1000 * 1000
    /** @brief Gets the number of bytes from amount of megabytes (MiB) (1000 * 1000) */
    #define MEGABYTES(amount) amount * 1000 * 1000
    /** @brief Gets the number of bytes from amount of kilobytes (KiB) (1000) */
    #define KILOBYTES(amount) amount * 1000

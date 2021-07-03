package tachiyomi.decoder

data class ImageType internal constructor(
  val format: Format,
  val isAnimated: Boolean
) {

  // Called from JNI
  internal constructor(
    format: Int,
    isAnimated: Boolean
  ) : this(Format.from(format), isAnimated)
}

enum class Format {
  Jpeg, Png, Webp, Gif, Heif, Avif, Jxl;

  internal companion object {
    fun from(value: Int): Format {
      // These values must match the ones returned in [java_wrapper.cpp]
      return when (value) {
        0 -> Jpeg
        1 -> Png
        2 -> Webp
        3 -> Gif
        4 -> Heif
        5 -> Avif
        6 -> Jxl
        else -> error("Invalid format code")
      }
    }
  }
}

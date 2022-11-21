package tachiyomi.decoder

import android.graphics.Bitmap
import android.graphics.Rect
import java.io.InputStream
import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.locks.ReentrantReadWriteLock
import kotlin.concurrent.read
import kotlin.concurrent.write

class ImageDecoder private constructor(
  private val nativePtr: Long,
  val width: Int,
  val height: Int
) {

  var isRecycled = false
    private set

  private var shouldRecycle = false
  private var decoding = AtomicInteger()
  private val lock = ReentrantReadWriteLock()

  fun decode(
    region: Rect = Rect(0, 0, width, height),
    rgb565: Boolean = true,
    sampleSize: Int = 1,
    applyColorManagement: Boolean = false,
    displayProfile: ByteArray? = null,
  ): Bitmap? {
    checkValidInput(region, sampleSize)
    return try {
      lock.read {
        decoding.incrementAndGet()
        require(!isRecycled) { "The decoder has been recycled" }
      }
      nativeDecode(
        nativePtr, rgb565, sampleSize, region.left, region.top,
        region.width(), region.height(), applyColorManagement, displayProfile
      )
    } finally {
      val currentDecoding = decoding.decrementAndGet()
      if (currentDecoding == 0) {
        checkRecycle()
      }
    }
  }

  private fun checkValidInput(region: Rect, sampleSize: Int) {
    val x = region.left
    val y = region.top
    val width = region.width()
    val height = region.height()

    if (x < 0 || x > this.width ||
      y < 0 || y > this.height ||
      width <= 0 || width + x > this.width ||
      height <= 0 || height + y > this.height
    ) {
      throw IllegalStateException("Requested region is invalid")
    }

    if (!(sampleSize > 0 && Integer.bitCount(sampleSize) == 1)) {
      throw IllegalStateException("Sample size must be a power of 2 but got $sampleSize")
    }
  }

  fun recycle() {
    if (isRecycled) return
    shouldRecycle = true
    checkRecycle()
  }

  private fun checkRecycle() {
    lock.write {
      if (!isRecycled && shouldRecycle && decoding.get() == 0) {
        nativeRecycle(nativePtr)
        isRecycled = true
      }
    }
  }

  protected fun finalize() {
    recycle()
  }

  private external fun nativeDecode(
    nativePtr: Long,
    rgb565: Boolean,
    sampleSize: Int,
    x: Int,
    y: Int,
    width: Int,
    height: Int,
    applyColorManagement: Boolean,
    displayProfile: ByteArray?,
  ): Bitmap?

  private external fun nativeRecycle(nativePtr: Long)

  companion object {
    init {
      System.loadLibrary("imagedecoder")
    }

    fun newInstance(
      stream: InputStream,
      cropBorders: Boolean = false,
    ) : ImageDecoder? {
      return stream.use { nativeNewInstance(it, cropBorders) }
    }

    fun findType(bytes: ByteArray): ImageType? {
      return nativeFindType(bytes)
    }

    @JvmStatic
    private external fun nativeNewInstance(
      stream: InputStream,
      cropBorders: Boolean = false,
    ) : ImageDecoder?

    @JvmStatic
    private external fun nativeFindType(bytes: ByteArray): ImageType?

    @JvmStatic
    private fun createBitmap(width: Int, height: Int, rgb565: Boolean): Bitmap? {
      return try {
        val config = if (rgb565) Bitmap.Config.RGB_565 else Bitmap.Config.ARGB_8888
        Bitmap.createBitmap(width, height, config)
      } catch (e: OutOfMemoryError) {
        null
      }
    }
  }
}

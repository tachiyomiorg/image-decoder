package tachiyomi.decoder

import android.graphics.Bitmap
import android.graphics.Rect
import java.io.InputStream
import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.locks.ReentrantReadWriteLock
import kotlin.concurrent.read
import kotlin.concurrent.write

data class ImageDecoder(
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
    sampleSize: Int = 1
  ): Bitmap? {
    return try {
      lock.read {
        decoding.incrementAndGet()
        checkRegionBounds(region.left, region.top, region.width(), region.height())
        require(!isRecycled) { "The decoder has been recycled" }
      }
      nativeDecode(
        nativePtr, rgb565, sampleSize,
        region.left, region.top, region.width(), region.height()
      )
    } finally {
      val currentDecoding = decoding.decrementAndGet()
      if (currentDecoding == 0) {
        checkRecycle()
      }
    }
  }

  private fun checkRegionBounds(x: Int, y: Int, width: Int, height: Int) {
    if (x < 0 || x > this.width ||
      y < 0 || y > this.height ||
      width <= 0 || width + x > this.width ||
      height <= 0 || height + y > this.height
    ) {
      throw IllegalStateException("Requested region is invalid")
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
    height: Int
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
      return nativeNewInstance(stream, cropBorders)
    }

    @JvmStatic
    private external fun nativeNewInstance(
      stream: InputStream,
      cropBorders: Boolean = false,
    ) : ImageDecoder?

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

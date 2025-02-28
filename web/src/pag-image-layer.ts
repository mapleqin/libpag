import { PAGModule } from './binding';
import { PAGLayer } from './pag-layer';
import { destroyVerify, wasmAwaitRewind } from './utils/decorators';
import { proxyVector } from './utils/type-utils';

import type { PAGImage } from './pag-image';
import type { PAGVideoRange } from './types';

@destroyVerify
@wasmAwaitRewind
export class PAGImageLayer extends PAGLayer {
  public static Make(width: number, height: number, duration: number): PAGImageLayer {
    return new PAGImageLayer(PAGModule._PAGImageLayer._Make(width, height, duration));
  }

  /**
   * Returns the content duration in microseconds, which indicates the minimal length required for
   * replacement.
   */
  public contentDuration(): number {
    return this.wasmIns._contentDuration() as number;
  }
  /**
   * Returns the time ranges of the source video for replacement.
   */
  public getVideoRanges() {
    return proxyVector(this.wasmIns._getVideoRanges(), (wasmIns) => wasmIns as PAGVideoRange);
  }
  /**
   * [Deprecated]
   * Replace the original image content with the specified PAGImage object.
   * Passing in null for the image parameter resets the layer to its default image content.
   * The replaceImage() method modifies all associated PAGImageLayers that have the same
   * editableIndex to this layer.
   *
   * @param image The PAGImage object to replace with.
   */
  public replaceImage(pagImage: PAGImage) {
    this.wasmIns._replaceImage(pagImage.wasmIns);
  }
  /**
   * Replace the original image content with the specified PAGImage object.
   * Passing in null for the image parameter resets the layer to its default image content.
   * The setImage() method only modifies the content of the calling PAGImageLayer.
   *
   * @param image The PAGImage object to replace with.
   */
  public setImage(pagImage: PAGImage) {
    this.wasmIns._setImage(pagImage.wasmIns);
  }
  /**
   * Converts the time from the PAGImageLayer's timeline to the replacement content's timeline. The
   * time is in microseconds.
   */
  public layerTimeToContent(layerTime: number): number {
    return this.wasmIns._layerTimeToContent(layerTime) as number;
  }
  /**
   * Converts the time from the replacement content's timeline to the PAGLayer's timeline. The time
   * is in microseconds.
   */
  public contentTimeToLayer(contentTime: number): number {
    return this.wasmIns._contentTimeToLayer(contentTime) as number;
  }
  /**
   * The default image data of this layer, which is webp format.
   */
  public imageBytes(): Uint8Array | null {
    return this.wasmIns._imageBytes();
  }
}

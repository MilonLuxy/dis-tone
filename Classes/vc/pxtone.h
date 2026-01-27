#ifdef PXTONEDLL_EXPORTS
#define DLLAPI __declspec(dllexport) // DLL を作る側は export として
#else
#define DLLAPI __declspec(dllimport) // DLL を使う側は import として
#endif

// C++ でも C でも使えるようにする
#ifdef __cplusplus
extern "C"{
#endif



typedef BOOL (* PXTONEPLAY_CALLBACK)( long clock, BOOL bEnd ); // -v.0.7.1.N
typedef bool (*   PXTNPLAY_CALLBACK)( int  clock            ); //  v.0.8.N.N-

// 以下 pxtone関数群 ==========================================

// @brief pxtone を生成します。
// @param[in] hWnd:　　　　 ウインドウハンドルを渡してください
// @param[in] channel_num:　のチャンネル数を指定してください。 ( 1:モノラル / 2:ステレオ )
// @param[in] sps:　　　　　 秒間サンプリングレートです。　　　( 11025 / 22050 / 44100 )
// @param[in] bps:　　　　　１サンプルを表現するビット数です。 ( 8 / 16 )
// @param[in] buffer_sec:　　 曲を再生するのに使用するバッファサイズを秒で指定します。( 推奨 0.1 )
// @param[in] bDirectSound:　( true -> DirectSound / false -> WAVEMAPPER )
// @param[in] pProc:　　　　 サンプリング毎に呼ばれる関数です。NULL でかまいません。
// @returns pxtone の準備 ( true / false )
bool DLLAPI pxtn_Ready( HWND hWnd, int channel_num, int sps, int bps, float buffer_sec, bool bDirectSound, PXTNPLAY_CALLBACK pProc );

// @brief pxtone を再設定します。
// @param[in] hWnd:　　　　 ウインドウハンドルを渡してください
// @param[in] channel_num:　チャンネル数
// @param[in] sps:　　　　　 秒間サンプル
// @param[in] bps:　　　　　ビット数
// @param[in] buffer_sec:　　 バッファサイズを秒
// @param[in] bDirectSound:　( true -> DirectSound / false -> WAVEMAPPER )
// @param[in] pProc:　　　　 コールバック関数
// @returns pxtone の準備 ( true / false )
bool DLLAPI pxtn_Reset( HWND hWnd, int channel_num, int sps, int bps, float buffer_sec, bool bDirectSound, PXTNPLAY_CALLBACK pProc );

// @brief pxtone で生成されたDirectSoundを取得する。
// @brief 取得したDirectSoundは自分でリリースしないように注意してください。
// @returns DirectSoundのポインタ(LPDIRECTSOUND)
void DLLAPI *pxtn_GetDirectSound( void );

// @brief ラストエラー文字列取得
// @returns エラーのメッセージ
const char DLLAPI *pxtn_GetLastError( void );

// @brief pxtone の音質を取得します
// @param[out] p_channel_num:　　チャンネル数です。
// @param[out] p_sps: 　　　　　　秒間サンプリングレートです。
// @param[out] p_bps:　　　　　　１サンプルを表現するビット数です。
// @param[out] p_sample_per_buf:　サンプリングのバッファに指定られたサイズ。
void DLLAPI pxtn_GetQuality( int *p_channel_num, int *p_sps, int *p_bps, int *p_sample_per_buf );

// @brief リソースバージョンを取得
// @param[out] p1: vX.0.0.0
// @param[out] p2: v0.X.0.0
// @param[out] p3: v0.0.X.0
// @param[out] p4: v0.0.0.X
// @returns すべてのパラメータを一度 (vX.X.X.X)
int DLLAPI pxtn_GetVersion( int *p1, int *p2, int *p3, int *p4 );

// @brief pxtone を開放します
// @returns pxtone の開放 ( true / false )
bool DLLAPI pxtn_Release( void );

// @brief 曲を読み込みます (ファイル・リソースから)
// @param[in] hModule:　リソースから読む場合はモジュールハンドルを指定します。NULL でも問題ないかも。
// @param[in] type_name: リソースから読む場合はリソースの種類名。外部ファイルを読む場合は NULL。
// @param[in] file_name:　ファイルパスもしくはリソース名。
// @returns 曲の読み込み ( true / false )
bool DLLAPI pxtn_Tune_Load( HMODULE hModule, const char *type_name, const char *file_name );

// @brief 曲を読み込む (メモリから)
// @param[in] p:　 リソースのデータ
// @param[in] size: リソースのサイズ
// @returns 曲の読み込み ( true / false )
bool DLLAPI pxtn_Tune_Read( void *p, int size );

// @brief 曲を解放します
// @returns 曲の解放 ( true / false )
bool DLLAPI pxtn_Tune_Release( void );

// @brief 曲を再生します
// @param[in] start_sample: 開始位置です。主に Stop や Fadeout で取得した値を設定します。0 で最初から。
// @param[in] fadein_msec: フェードインする場合はここに時間（ミリ秒）を指定します。
// @param[in] volume:　　 曲のボリューム。
// @returns 曲の再生 ( true / false )
bool DLLAPI pxtn_Tune_Start( int start_sample, int fadein_msec, float volume );

// @brief フェードアウトスイッチを入れて現在再生サンプルを取得します
// @param[in] msec: フェードアウト時間（ミリ秒）
// @returns 停止時の曲の位置を取得します。
int DLLAPI pxtn_Tune_Fadeout( int msec );

// @brief 曲のボリュームを設定します。1.0 が最大で、0.5 が半分です。
// @param[in] v: ボリュームを設定する。
void DLLAPI pxtn_Tune_SetVolume( float v );

// @brief 曲を停止して現在再生サンプルを取得
// @returns 停止時の曲の位置を取得します。
int DLLAPI pxtn_Tune_Stop( void );

// @brief 再生中かどうかを調べます
// @returns 再生の状態 ( true / false )
bool DLLAPI pxtn_Tune_IsStreaming( void );

// @brief ループ再生の ON/OFF を切り替えます
// @param[in] bLoop: 再生をループする / 再生をループしない
void DLLAPI pxtn_Tune_SetLoop( bool bLoop );

// @brief 曲の情報を取得します
// @param[out] p_beat_num:　 拍子数
// @param[out] p_beat_tempo: ビートテンポ
// @param[out] p_beat_clock:　クロック
// @param[out] p_meas_num:　小節数
void DLLAPI pxtn_Tune_GetInformation( int *p_beat_num, float *p_beat_tempo, int *p_beat_clock, int *p_meas_num );

// @brief リピート小節を取得します
// @returns リピート小節
int DLLAPI pxtn_Tune_GetRepeatMeas( void );

// 有効演奏小節を取得します (LASTイベント小節。無ければ最終小節)
// @returns 小節総数
int DLLAPI pxtn_Tune_GetPlayMeas( void );

// @brief 曲の名称を取得します
// @returns 名称文字列
const char DLLAPI *pxtn_Tune_GetName( void );

// @brief 曲のコメントを取得します
// @returns コメント文字列
const char DLLAPI *pxtn_Tune_GetComment( void );

// @brief 指定のアドレスに再生バッファを書き込みます
// @brief .
// @brief １. この関数を使用する場合は pxtone_Ready() の引数 buffer_sec には 0 を設定してください。
//            pxtone のストリーミング機能が無効になります。
// @brief ２. 曲のロードと pxtone_Tune_Start() を終えてからこの関数を呼び出してください。
// @brief ３. sample_num はサイズではなくてサンプル数です。
// @brief 例: 11025hz 2ch 8bit を１秒吐き出す場合、sample_num は 11025 を指定する。
//            p には 22050バイト分の再生バッファが書き込まれる。
// @brief ４. ストリーミング機能が有効な時と同様に pxtone_Tune_Fadeout() 等の関数が使えます。
// @param[out] p:　　　　　 再生バッファを吐き出すアドレスです
// @param[in] sample_num: 書き込むサンプル数です
// @returns まだ続きがある場合は TRUE それ以外は FALSE
bool DLLAPI pxtn_Tune_Vomit( void *p, int sample_num );

// @brief ユニットの消音を適用する
// @param[out] unit:　　 対象のユニット
// @param[out] bMute:　消音する / 消音しない
// @returns 消音の適用 ( true / false )
bool DLLAPI pxtn_Tune_MuteUnit( int unit, bool bMute );




// ピストンノイズを生成します
typedef struct
{
    unsigned char *p_buf;
    int            size ;
}PXTONENOISEBUFFER;

// @brief ptnoise 生成のための pxtnFrequency を初期化
// @brief pxtone_Ready() を既に呼んでいる場合は必要ありません。
void DLLAPI pxtn_Noise_Initialize( void );

// @brief ptnoise のバッファを開放します
// @param[in] p_noise: 生成された ptnoise バッファ
void DLLAPI pxtn_Noise_Release( PXTONENOISEBUFFER *p_noise );

// @brief ptnoise で音声データを生成します (ファイル・リソースから)
// @param[in] name:　　　　 リソース名　　 を設定。外部ファイルの場合はファイルパス。
// @param[in] type:　　　　　リソースタイプ名を設定。外部ファイルの場合はNULL。
// @param[in] channel_num:　ptnoise のチャンネル数
// @param[in] sps:　　　　　 秒間サンプル
// @param[in] bps:　　　　　ビット数
// @returns 生成された ptnoise バッファ
PXTONENOISEBUFFER DLLAPI *pxtn_Noise_Create( const char *name, const char *type, int channel_num, int sps, int bps );

// @brief ptnoise で音声データを生成します (メモリから)
// @param[in] p_designdata:　　ノイズデザインが入ったバッファへのポインタ
// @param[in] designdata_size:　ノイズデザインが入ったバッファのサイズ
// @param[in] channel_num:　　ptnoise のチャンネル数
// @param[in] sps:　　　　　　 秒間サンプル
// @param[in] bps:　　　　　　ビット数
// @returns 生成された ptnoise バッファ
PXTONENOISEBUFFER DLLAPI *pxtn_Noise_Create_FromMemory( const char *p_designdata, int designdata_size, int channel_num, int sps, int bps );










// Overload calls ---------------------------------------------------------------------

// @brief Initializes pxtone.
// @param hWnd:　　　  Window Handle of the application
// @param channel_num: Specifies the number of channels. ( 1: Mono / 2: Stereo )
// @param sps:　　　　   Sample rate per seconds. ( 11025 / 22050 / 44100 )
// @param bps:　　　　  Number of bits representing one sample. ( 8 / 16 )
// @param buffer_sec:　   Specifies the buffer size in seconds to play the song. ( recommended: 0.1 )
// @param bDirectSound: ( true -> DirectSound / false -> WAVEMAPPER )
// @param pProc:　　　   A function called during playback. Can be set to NULL.
// @returns Pxtone preparation ( true / false )
BOOL DLLAPI pxtone_Ready( HWND hWnd, long channel_num, long sps, long bps, float buffer_sec, BOOL bDirectSound, PXTONEPLAY_CALLBACK pProc );

// @brief Reconfigures pxtone.
// @param hWnd:　　　  Window Handle of the application
// @param channel_num: Specifies the number of channels. ( 1:Mono / 2:Stereo )
// @param sps:　　　　   Sample rate per seconds. ( 11025 / 22050 / 44100 )
// @param bps:　　　　  Number of bits representing one sample. ( 8 / 16 )
// @param buffer_sec:　   Specifies the buffer size in seconds to play the song. ( recommended: 0.1 )
// @param bDirectSound: ( true -> DirectSound / false -> WAVEMAPPER )
// @param pProc:　　　   A function called during playback. Can be set to NULL.
// @returns Pxtone preparation ( true / false )
BOOL DLLAPI pxtone_Reset( HWND hWnd, long channel_num, long sps, long bps, float buffer_sec, BOOL bDirectSound, PXTONEPLAY_CALLBACK pProc ); BOOL DLLAPI pxtone_ResetSampling( HWND hWnd, long channel_num, long sps, long bps, float buffer_sec, BOOL bDirectSound, PXTONEPLAY_CALLBACK pProc );

// @brief Retrieves the DirectSound generated by pxtone.
// @brief Be careful not to release this DirectSound if you have acquired it!
// @returns Pointer of the DirectSound (LPDIRECTSOUND)
void DLLAPI *pxtone_GetDirectSound( void );

// @brief Retrieves the last occurred error.
// @returns Error message
const char DLLAPI *pxtone_GetLastError( void ); const char DLLAPI *pxtone_Tune_GetLastError( void );

// @brief Retrieves the sound quality from pxtone.
// @param p_channel_num:　 Channel number
// @param p_sps: 　　　　　  Sample rate
// @param p_bps:　　　　　  Bits per sample
// @param p_sample_per_buf: Allocated size of the sampling buffer
void DLLAPI pxtone_GetQuality( long *p_channel_num, long *p_sps, long *p_bps, long *p_sample_per_buf );

// @brief Gets the version number of the DLL.
// @returns Version number, or '-1' if something go wrong
long DLLAPI pxtone_GetVersion( void );

// @brief Releases pxtone.
// @returns Release proccess ( true / false )
BOOL DLLAPI pxtone_Release( void );

// @brief Loads the song. (from file / resource)
// @param hModule:　If you're gonna read from a resource, specify the module handle. Otherwise, NULL is fine.
// @param type_name: The resource type name if is a resource, or NULL if is an external file.
// @param file_name:　The file path or resource name.
// @returns Reading proccess ( true / false )
BOOL DLLAPI pxtone_Tune_Load( HMODULE hModule, const char *type_name, const char *file_name );

// @brief Loads the song. (from memory)
// @param p:　 Resource data
// @param size: Resource size
// @returns Reading proccess ( true / false )
BOOL DLLAPI pxtone_Tune_Read( void *p, long size );

// @brief Releases song data from memory.
// @returns Release proccess ( true / false )
BOOL DLLAPI pxtone_Tune_Release( void );

// @brief Plays the loaded song.
// @param start_sample: The playback position. Can be set to a value got from Stop or Fadeout. 0 is the beginning.
// @param fadein_msec: If you want to fade in, specify the time here. (milliseconds)
// @param volume:　　  Song volume.
// @returns Starting proccess ( true / false )
BOOL DLLAPI pxtone_Tune_Start( long start_sample, long fadein_msec ); BOOL DLLAPI pxtone_Tune_Play( long start_sample, long fade_msec );

// @brief Fades the song out, and gets the current playback position.
// @param msec: Fadeout time (miliseconds)
// @returns Playback position when stopped
long DLLAPI pxtone_Tune_Fadeout( long msec );

// @brief Sets the song volume. 1.0 is max volume and 0.5 is half.
// @param v: Desired value
void DLLAPI pxtone_Tune_SetVolume( float v );

// @brief Stops the song, and gets the current playback position.
// @returns Playback position when stopped
long DLLAPI pxtone_Tune_Stop( void );

// @brief Checks if it's currently playing.
// @returns Playback state ( true / false )
BOOL DLLAPI pxtone_Tune_IsStreaming( void ); BOOL DLLAPI pxtone_Tune_IsPlaying( void );

// @brief Toggles loop playback ON/OFF.
// @param bLoop: Loop / Play once ( true / false )
void DLLAPI pxtone_Tune_SetLoop( BOOL bLoop );

// @brief Gets song information.
// @param p_beat_num:　 Number of beats
// @param p_beat_tempo: Song BPM
// @param p_beat_clock:　Beat clock
// @param p_meas_num:　Total measures of the song
void DLLAPI pxtone_Tune_GetInformation( long *p_beat_num, float *p_beat_tempo, long *p_beat_clock, long *p_meas_num );

// @brief Gets the repeat measure.
// @returns Number of the repeat measure (REPEAT event)
long DLLAPI pxtone_Tune_GetRepeatMeas( void );

// @brief Gets all valid measures from the song. (LAST event, or the last measure from the file)
// @returns Total number of measures
long DLLAPI pxtone_Tune_GetPlayMeas( void );

// @brief Gets the song name.
// @returns Name string
const char DLLAPI *pxtone_Tune_GetName( void );

// @brief Gets the song comment.
// @returns Comment string
const char DLLAPI *pxtone_Tune_GetComment( void );

// @brief Writes the playback buffer to the specified address.
// @brief .
// @brief 1. To use this function, set the 'buffer_sec' from pxtone_Ready() to 0.
//           The streaming procedure from pxtone will be disabled.
// @brief 2. Then, call this function after loading the song and calling pxtone_Tune_Start().
// @brief 3. The 'sample_num' here is the sample NUMBER, not the size.
// @brief Ex: To output 11025hz 2ch 8bit for 1 second, set 'sample_num' to '11025'.
//            '22050' bytes of the playback buffer will then be written to 'p'.
// @brief 4. Functions like pxtone_Tune_Fadeout() can be used the same way the streaming procedure does.
// @param p:　　　　　 Address to dump the playback buffer
// @param sample_num: The number of samples to write
// @returns If there is more to write, TRUE; otherwise, FALSE
BOOL DLLAPI pxtone_Tune_Vomit( void *p, long sample_num );

// @brief Applies mute to a unit.
// @param unit:　  Target unit
// @param bMute: Mute / Unmute
// @returns Mute operation ( true / false )
BOOL DLLAPI pxtone_Tune_MuteUnit( long unit, BOOL bMute );

// @brief Initializes 'ptnoise' generation.
// @brief Not necessary if you have already called pxtone_Ready().
void DLLAPI pxtone_Noise_Initialize( void );

// @brief Releases the generated 'ptnoise' buffer from memory.
// @param p_noise: 'ptnoise' struct
void DLLAPI pxtone_Noise_Release( PXTONENOISEBUFFER *p_noise );

// @brief Generates audio data with ptNoise. (from a file or resource)
// @param name:　　　   The resource         name. If it's a file, set the file path.
// @param type:　　　　 The resource type name. If it's a file, set it to NULL.
// @param channel_num: Channel number of the 'ptnoise'
// @param sps:　　　　   Sample rate
// @param bps:　　　　   Bits per sample
// @returns Generated 'ptnoise' buffer
PXTONENOISEBUFFER DLLAPI *pxtone_Noise_Create( const char *name, const char *type, long channel_num, long sps, long bps );

// @brief ptnoise Generates audio data with ptNoise. (from memory)
// @param p_designdata:　 Pointer of the buffer containing the noise design
// @param designdata_size: Size of the buffer containing the noise design
// @param channel_num:　  Channel number of the 'ptnoise'
// @param sps:　　　　　   Sample rate
// @param bps:　　　　　   Bits per sample
// @returns Generated 'ptnoise' buffer
PXTONENOISEBUFFER DLLAPI *pxtone_Noise_Create_FromMemory( const char *p_designdata, int designdata_size, long channel_num, long sps, long bps );
// ------------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

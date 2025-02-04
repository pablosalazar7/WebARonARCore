// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is auto-generated from
// gpu/command_buffer/build_gles2_cmd_buffer.py
// It's formatted by clang-format using chromium coding style:
//    clang-format -i -style=chromium filename
// DO NOT EDIT!

#ifndef GPU_COMMAND_BUFFER_COMMON_GLES2_CMD_IDS_AUTOGEN_H_
#define GPU_COMMAND_BUFFER_COMMON_GLES2_CMD_IDS_AUTOGEN_H_

#define GLES2_COMMAND_LIST(OP)                                       \
  OP(ActiveTexture)                                        /* 256 */ \
  OP(AttachShader)                                         /* 257 */ \
  OP(BindAttribLocationBucket)                             /* 258 */ \
  OP(BindBuffer)                                           /* 259 */ \
  OP(BindBufferBase)                                       /* 260 */ \
  OP(BindBufferRange)                                      /* 261 */ \
  OP(BindFramebuffer)                                      /* 262 */ \
  OP(BindRenderbuffer)                                     /* 263 */ \
  OP(BindSampler)                                          /* 264 */ \
  OP(BindTexture)                                          /* 265 */ \
  OP(UpdateTextureExternalOes)                             /* 266 */ \
  OP(BindTransformFeedback)                                /* 267 */ \
  OP(BlendColor)                                           /* 268 */ \
  OP(BlendEquation)                                        /* 269 */ \
  OP(BlendEquationSeparate)                                /* 270 */ \
  OP(BlendFunc)                                            /* 271 */ \
  OP(BlendFuncSeparate)                                    /* 272 */ \
  OP(BufferData)                                           /* 273 */ \
  OP(BufferSubData)                                        /* 274 */ \
  OP(CheckFramebufferStatus)                               /* 275 */ \
  OP(Clear)                                                /* 276 */ \
  OP(ClearBufferfi)                                        /* 277 */ \
  OP(ClearBufferfvImmediate)                               /* 278 */ \
  OP(ClearBufferivImmediate)                               /* 279 */ \
  OP(ClearBufferuivImmediate)                              /* 280 */ \
  OP(ClearColor)                                           /* 281 */ \
  OP(ClearDepthf)                                          /* 282 */ \
  OP(ClearStencil)                                         /* 283 */ \
  OP(ClientWaitSync)                                       /* 284 */ \
  OP(ColorMask)                                            /* 285 */ \
  OP(CompileShader)                                        /* 286 */ \
  OP(CompressedTexImage2DBucket)                           /* 287 */ \
  OP(CompressedTexImage2D)                                 /* 288 */ \
  OP(CompressedTexSubImage2DBucket)                        /* 289 */ \
  OP(CompressedTexSubImage2D)                              /* 290 */ \
  OP(CompressedTexImage3DBucket)                           /* 291 */ \
  OP(CompressedTexImage3D)                                 /* 292 */ \
  OP(CompressedTexSubImage3DBucket)                        /* 293 */ \
  OP(CompressedTexSubImage3D)                              /* 294 */ \
  OP(CopyBufferSubData)                                    /* 295 */ \
  OP(CopyTexImage2D)                                       /* 296 */ \
  OP(CopyTexSubImage2D)                                    /* 297 */ \
  OP(CopyTexSubImage3D)                                    /* 298 */ \
  OP(CreateProgram)                                        /* 299 */ \
  OP(CreateShader)                                         /* 300 */ \
  OP(CullFace)                                             /* 301 */ \
  OP(DeleteBuffersImmediate)                               /* 302 */ \
  OP(DeleteFramebuffersImmediate)                          /* 303 */ \
  OP(DeleteProgram)                                        /* 304 */ \
  OP(DeleteRenderbuffersImmediate)                         /* 305 */ \
  OP(DeleteSamplersImmediate)                              /* 306 */ \
  OP(DeleteSync)                                           /* 307 */ \
  OP(DeleteShader)                                         /* 308 */ \
  OP(DeleteTexturesImmediate)                              /* 309 */ \
  OP(DeleteTransformFeedbacksImmediate)                    /* 310 */ \
  OP(DepthFunc)                                            /* 311 */ \
  OP(DepthMask)                                            /* 312 */ \
  OP(DepthRangef)                                          /* 313 */ \
  OP(DetachShader)                                         /* 314 */ \
  OP(Disable)                                              /* 315 */ \
  OP(DisableVertexAttribArray)                             /* 316 */ \
  OP(DrawArrays)                                           /* 317 */ \
  OP(DrawElements)                                         /* 318 */ \
  OP(Enable)                                               /* 319 */ \
  OP(EnableVertexAttribArray)                              /* 320 */ \
  OP(FenceSync)                                            /* 321 */ \
  OP(Finish)                                               /* 322 */ \
  OP(Flush)                                                /* 323 */ \
  OP(FramebufferRenderbuffer)                              /* 324 */ \
  OP(FramebufferTexture2D)                                 /* 325 */ \
  OP(FramebufferTextureLayer)                              /* 326 */ \
  OP(FrontFace)                                            /* 327 */ \
  OP(GenBuffersImmediate)                                  /* 328 */ \
  OP(GenerateMipmap)                                       /* 329 */ \
  OP(GenFramebuffersImmediate)                             /* 330 */ \
  OP(GenRenderbuffersImmediate)                            /* 331 */ \
  OP(GenSamplersImmediate)                                 /* 332 */ \
  OP(GenTexturesImmediate)                                 /* 333 */ \
  OP(GenTransformFeedbacksImmediate)                       /* 334 */ \
  OP(GetActiveAttrib)                                      /* 335 */ \
  OP(GetActiveUniform)                                     /* 336 */ \
  OP(GetActiveUniformBlockiv)                              /* 337 */ \
  OP(GetActiveUniformBlockName)                            /* 338 */ \
  OP(GetActiveUniformsiv)                                  /* 339 */ \
  OP(GetAttachedShaders)                                   /* 340 */ \
  OP(GetAttribLocation)                                    /* 341 */ \
  OP(GetBooleanv)                                          /* 342 */ \
  OP(GetBufferParameteri64v)                               /* 343 */ \
  OP(GetBufferParameteriv)                                 /* 344 */ \
  OP(GetError)                                             /* 345 */ \
  OP(GetFloatv)                                            /* 346 */ \
  OP(GetFragDataLocation)                                  /* 347 */ \
  OP(GetFramebufferAttachmentParameteriv)                  /* 348 */ \
  OP(GetInteger64v)                                        /* 349 */ \
  OP(GetIntegeri_v)                                        /* 350 */ \
  OP(GetInteger64i_v)                                      /* 351 */ \
  OP(GetIntegerv)                                          /* 352 */ \
  OP(GetInternalformativ)                                  /* 353 */ \
  OP(GetProgramiv)                                         /* 354 */ \
  OP(GetProgramInfoLog)                                    /* 355 */ \
  OP(GetRenderbufferParameteriv)                           /* 356 */ \
  OP(GetSamplerParameterfv)                                /* 357 */ \
  OP(GetSamplerParameteriv)                                /* 358 */ \
  OP(GetShaderiv)                                          /* 359 */ \
  OP(GetShaderInfoLog)                                     /* 360 */ \
  OP(GetShaderPrecisionFormat)                             /* 361 */ \
  OP(GetShaderSource)                                      /* 362 */ \
  OP(GetString)                                            /* 363 */ \
  OP(GetSynciv)                                            /* 364 */ \
  OP(GetTexParameterfv)                                    /* 365 */ \
  OP(GetTexParameteriv)                                    /* 366 */ \
  OP(GetTransformFeedbackVarying)                          /* 367 */ \
  OP(GetUniformBlockIndex)                                 /* 368 */ \
  OP(GetUniformfv)                                         /* 369 */ \
  OP(GetUniformiv)                                         /* 370 */ \
  OP(GetUniformuiv)                                        /* 371 */ \
  OP(GetUniformIndices)                                    /* 372 */ \
  OP(GetUniformLocation)                                   /* 373 */ \
  OP(GetVertexAttribfv)                                    /* 374 */ \
  OP(GetVertexAttribiv)                                    /* 375 */ \
  OP(GetVertexAttribIiv)                                   /* 376 */ \
  OP(GetVertexAttribIuiv)                                  /* 377 */ \
  OP(GetVertexAttribPointerv)                              /* 378 */ \
  OP(Hint)                                                 /* 379 */ \
  OP(InvalidateFramebufferImmediate)                       /* 380 */ \
  OP(InvalidateSubFramebufferImmediate)                    /* 381 */ \
  OP(IsBuffer)                                             /* 382 */ \
  OP(IsEnabled)                                            /* 383 */ \
  OP(IsFramebuffer)                                        /* 384 */ \
  OP(IsProgram)                                            /* 385 */ \
  OP(IsRenderbuffer)                                       /* 386 */ \
  OP(IsSampler)                                            /* 387 */ \
  OP(IsShader)                                             /* 388 */ \
  OP(IsSync)                                               /* 389 */ \
  OP(IsTexture)                                            /* 390 */ \
  OP(IsTransformFeedback)                                  /* 391 */ \
  OP(LineWidth)                                            /* 392 */ \
  OP(LinkProgram)                                          /* 393 */ \
  OP(PauseTransformFeedback)                               /* 394 */ \
  OP(PixelStorei)                                          /* 395 */ \
  OP(PolygonOffset)                                        /* 396 */ \
  OP(ReadBuffer)                                           /* 397 */ \
  OP(ReadPixels)                                           /* 398 */ \
  OP(ReleaseShaderCompiler)                                /* 399 */ \
  OP(RenderbufferStorage)                                  /* 400 */ \
  OP(ResumeTransformFeedback)                              /* 401 */ \
  OP(SampleCoverage)                                       /* 402 */ \
  OP(SamplerParameterf)                                    /* 403 */ \
  OP(SamplerParameterfvImmediate)                          /* 404 */ \
  OP(SamplerParameteri)                                    /* 405 */ \
  OP(SamplerParameterivImmediate)                          /* 406 */ \
  OP(Scissor)                                              /* 407 */ \
  OP(ShaderBinary)                                         /* 408 */ \
  OP(ShaderSourceBucket)                                   /* 409 */ \
  OP(StencilFunc)                                          /* 410 */ \
  OP(StencilFuncSeparate)                                  /* 411 */ \
  OP(StencilMask)                                          /* 412 */ \
  OP(StencilMaskSeparate)                                  /* 413 */ \
  OP(StencilOp)                                            /* 414 */ \
  OP(StencilOpSeparate)                                    /* 415 */ \
  OP(TexImage2D)                                           /* 416 */ \
  OP(TexImage3D)                                           /* 417 */ \
  OP(TexParameterf)                                        /* 418 */ \
  OP(TexParameterfvImmediate)                              /* 419 */ \
  OP(TexParameteri)                                        /* 420 */ \
  OP(TexParameterivImmediate)                              /* 421 */ \
  OP(TexStorage3D)                                         /* 422 */ \
  OP(TexSubImage2D)                                        /* 423 */ \
  OP(TexSubImage3D)                                        /* 424 */ \
  OP(TransformFeedbackVaryingsBucket)                      /* 425 */ \
  OP(Uniform1f)                                            /* 426 */ \
  OP(Uniform1fvImmediate)                                  /* 427 */ \
  OP(Uniform1i)                                            /* 428 */ \
  OP(Uniform1ivImmediate)                                  /* 429 */ \
  OP(Uniform1ui)                                           /* 430 */ \
  OP(Uniform1uivImmediate)                                 /* 431 */ \
  OP(Uniform2f)                                            /* 432 */ \
  OP(Uniform2fvImmediate)                                  /* 433 */ \
  OP(Uniform2i)                                            /* 434 */ \
  OP(Uniform2ivImmediate)                                  /* 435 */ \
  OP(Uniform2ui)                                           /* 436 */ \
  OP(Uniform2uivImmediate)                                 /* 437 */ \
  OP(Uniform3f)                                            /* 438 */ \
  OP(Uniform3fvImmediate)                                  /* 439 */ \
  OP(Uniform3i)                                            /* 440 */ \
  OP(Uniform3ivImmediate)                                  /* 441 */ \
  OP(Uniform3ui)                                           /* 442 */ \
  OP(Uniform3uivImmediate)                                 /* 443 */ \
  OP(Uniform4f)                                            /* 444 */ \
  OP(Uniform4fvImmediate)                                  /* 445 */ \
  OP(Uniform4i)                                            /* 446 */ \
  OP(Uniform4ivImmediate)                                  /* 447 */ \
  OP(Uniform4ui)                                           /* 448 */ \
  OP(Uniform4uivImmediate)                                 /* 449 */ \
  OP(UniformBlockBinding)                                  /* 450 */ \
  OP(UniformMatrix2fvImmediate)                            /* 451 */ \
  OP(UniformMatrix2x3fvImmediate)                          /* 452 */ \
  OP(UniformMatrix2x4fvImmediate)                          /* 453 */ \
  OP(UniformMatrix3fvImmediate)                            /* 454 */ \
  OP(UniformMatrix3x2fvImmediate)                          /* 455 */ \
  OP(UniformMatrix3x4fvImmediate)                          /* 456 */ \
  OP(UniformMatrix4fvImmediate)                            /* 457 */ \
  OP(UniformMatrix4x2fvImmediate)                          /* 458 */ \
  OP(UniformMatrix4x3fvImmediate)                          /* 459 */ \
  OP(UseProgram)                                           /* 460 */ \
  OP(ValidateProgram)                                      /* 461 */ \
  OP(VertexAttrib1f)                                       /* 462 */ \
  OP(VertexAttrib1fvImmediate)                             /* 463 */ \
  OP(VertexAttrib2f)                                       /* 464 */ \
  OP(VertexAttrib2fvImmediate)                             /* 465 */ \
  OP(VertexAttrib3f)                                       /* 466 */ \
  OP(VertexAttrib3fvImmediate)                             /* 467 */ \
  OP(VertexAttrib4f)                                       /* 468 */ \
  OP(VertexAttrib4fvImmediate)                             /* 469 */ \
  OP(VertexAttribI4i)                                      /* 470 */ \
  OP(VertexAttribI4ivImmediate)                            /* 471 */ \
  OP(VertexAttribI4ui)                                     /* 472 */ \
  OP(VertexAttribI4uivImmediate)                           /* 473 */ \
  OP(VertexAttribIPointer)                                 /* 474 */ \
  OP(VertexAttribPointer)                                  /* 475 */ \
  OP(Viewport)                                             /* 476 */ \
  OP(WaitSync)                                             /* 477 */ \
  OP(BlitFramebufferCHROMIUM)                              /* 478 */ \
  OP(RenderbufferStorageMultisampleCHROMIUM)               /* 479 */ \
  OP(RenderbufferStorageMultisampleEXT)                    /* 480 */ \
  OP(FramebufferTexture2DMultisampleEXT)                   /* 481 */ \
  OP(TexStorage2DEXT)                                      /* 482 */ \
  OP(GenQueriesEXTImmediate)                               /* 483 */ \
  OP(DeleteQueriesEXTImmediate)                            /* 484 */ \
  OP(QueryCounterEXT)                                      /* 485 */ \
  OP(BeginQueryEXT)                                        /* 486 */ \
  OP(BeginTransformFeedback)                               /* 487 */ \
  OP(EndQueryEXT)                                          /* 488 */ \
  OP(EndTransformFeedback)                                 /* 489 */ \
  OP(SetDisjointValueSyncCHROMIUM)                         /* 490 */ \
  OP(InsertEventMarkerEXT)                                 /* 491 */ \
  OP(PushGroupMarkerEXT)                                   /* 492 */ \
  OP(PopGroupMarkerEXT)                                    /* 493 */ \
  OP(GenVertexArraysOESImmediate)                          /* 494 */ \
  OP(DeleteVertexArraysOESImmediate)                       /* 495 */ \
  OP(IsVertexArrayOES)                                     /* 496 */ \
  OP(BindVertexArrayOES)                                   /* 497 */ \
  OP(SwapBuffers)                                          /* 498 */ \
  OP(GetMaxValueInBufferCHROMIUM)                          /* 499 */ \
  OP(EnableFeatureCHROMIUM)                                /* 500 */ \
  OP(MapBufferRange)                                       /* 501 */ \
  OP(UnmapBuffer)                                          /* 502 */ \
  OP(FlushMappedBufferRange)                               /* 503 */ \
  OP(ResizeCHROMIUM)                                       /* 504 */ \
  OP(GetRequestableExtensionsCHROMIUM)                     /* 505 */ \
  OP(RequestExtensionCHROMIUM)                             /* 506 */ \
  OP(GetProgramInfoCHROMIUM)                               /* 507 */ \
  OP(GetUniformBlocksCHROMIUM)                             /* 508 */ \
  OP(GetTransformFeedbackVaryingsCHROMIUM)                 /* 509 */ \
  OP(GetUniformsES3CHROMIUM)                               /* 510 */ \
  OP(DescheduleUntilFinishedCHROMIUM)                      /* 511 */ \
  OP(GetTranslatedShaderSourceANGLE)                       /* 512 */ \
  OP(PostSubBufferCHROMIUM)                                /* 513 */ \
  OP(CopyTextureCHROMIUM)                                  /* 514 */ \
  OP(CopySubTextureCHROMIUM)                               /* 515 */ \
  OP(CompressedCopyTextureCHROMIUM)                        /* 516 */ \
  OP(DrawArraysInstancedANGLE)                             /* 517 */ \
  OP(DrawElementsInstancedANGLE)                           /* 518 */ \
  OP(VertexAttribDivisorANGLE)                             /* 519 */ \
  OP(ProduceTextureCHROMIUMImmediate)                      /* 520 */ \
  OP(ProduceTextureDirectCHROMIUMImmediate)                /* 521 */ \
  OP(ConsumeTextureCHROMIUMImmediate)                      /* 522 */ \
  OP(CreateAndConsumeTextureINTERNALImmediate)             /* 523 */ \
  OP(BindUniformLocationCHROMIUMBucket)                    /* 524 */ \
  OP(BindTexImage2DCHROMIUM)                               /* 525 */ \
  OP(ReleaseTexImage2DCHROMIUM)                            /* 526 */ \
  OP(TraceBeginCHROMIUM)                                   /* 527 */ \
  OP(TraceEndCHROMIUM)                                     /* 528 */ \
  OP(DiscardFramebufferEXTImmediate)                       /* 529 */ \
  OP(LoseContextCHROMIUM)                                  /* 530 */ \
  OP(InsertFenceSyncCHROMIUM)                              /* 531 */ \
  OP(WaitSyncTokenCHROMIUM)                                /* 532 */ \
  OP(DrawBuffersEXTImmediate)                              /* 533 */ \
  OP(DiscardBackbufferCHROMIUM)                            /* 534 */ \
  OP(ScheduleOverlayPlaneCHROMIUM)                         /* 535 */ \
  OP(ScheduleCALayerSharedStateCHROMIUM)                   /* 536 */ \
  OP(ScheduleCALayerCHROMIUM)                              /* 537 */ \
  OP(ScheduleCALayerInUseQueryCHROMIUMImmediate)           /* 538 */ \
  OP(CommitOverlayPlanesCHROMIUM)                          /* 539 */ \
  OP(SwapInterval)                                         /* 540 */ \
  OP(FlushDriverCachesCHROMIUM)                            /* 541 */ \
  OP(MatrixLoadfCHROMIUMImmediate)                         /* 542 */ \
  OP(MatrixLoadIdentityCHROMIUM)                           /* 543 */ \
  OP(GenPathsCHROMIUM)                                     /* 544 */ \
  OP(DeletePathsCHROMIUM)                                  /* 545 */ \
  OP(IsPathCHROMIUM)                                       /* 546 */ \
  OP(PathCommandsCHROMIUM)                                 /* 547 */ \
  OP(PathParameterfCHROMIUM)                               /* 548 */ \
  OP(PathParameteriCHROMIUM)                               /* 549 */ \
  OP(PathStencilFuncCHROMIUM)                              /* 550 */ \
  OP(StencilFillPathCHROMIUM)                              /* 551 */ \
  OP(StencilStrokePathCHROMIUM)                            /* 552 */ \
  OP(CoverFillPathCHROMIUM)                                /* 553 */ \
  OP(CoverStrokePathCHROMIUM)                              /* 554 */ \
  OP(StencilThenCoverFillPathCHROMIUM)                     /* 555 */ \
  OP(StencilThenCoverStrokePathCHROMIUM)                   /* 556 */ \
  OP(StencilFillPathInstancedCHROMIUM)                     /* 557 */ \
  OP(StencilStrokePathInstancedCHROMIUM)                   /* 558 */ \
  OP(CoverFillPathInstancedCHROMIUM)                       /* 559 */ \
  OP(CoverStrokePathInstancedCHROMIUM)                     /* 560 */ \
  OP(StencilThenCoverFillPathInstancedCHROMIUM)            /* 561 */ \
  OP(StencilThenCoverStrokePathInstancedCHROMIUM)          /* 562 */ \
  OP(BindFragmentInputLocationCHROMIUMBucket)              /* 563 */ \
  OP(ProgramPathFragmentInputGenCHROMIUM)                  /* 564 */ \
  OP(GetBufferSubDataAsyncCHROMIUM)                        /* 565 */ \
  OP(CoverageModulationCHROMIUM)                           /* 566 */ \
  OP(BlendBarrierKHR)                                      /* 567 */ \
  OP(ApplyScreenSpaceAntialiasingCHROMIUM)                 /* 568 */ \
  OP(BindFragDataLocationIndexedEXTBucket)                 /* 569 */ \
  OP(BindFragDataLocationEXTBucket)                        /* 570 */ \
  OP(GetFragDataIndexEXT)                                  /* 571 */ \
  OP(UniformMatrix4fvStreamTextureMatrixCHROMIUMImmediate) /* 572 */ \
  OP(OverlayPromotionHintCHROMIUM)                         /* 573 */ \
  OP(SwapBuffersWithDamageCHROMIUM)                        /* 574 */

enum CommandId {
  kOneBeforeStartPoint =
      cmd::kLastCommonId,  // All GLES2 commands start after this.
#define GLES2_CMD_OP(name) k##name,
  GLES2_COMMAND_LIST(GLES2_CMD_OP)
#undef GLES2_CMD_OP
      kNumCommands,
  kFirstGLES2Command = kOneBeforeStartPoint + 1
};

#endif  // GPU_COMMAND_BUFFER_COMMON_GLES2_CMD_IDS_AUTOGEN_H_

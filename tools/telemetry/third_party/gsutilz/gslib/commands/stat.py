# -*- coding: utf-8 -*-
# Copyright 2013 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Implementation of Unix-like stat command for cloud storage providers."""

from __future__ import absolute_import

import logging

from gslib.bucket_listing_ref import BucketListingObject
from gslib.cloud_api import AccessDeniedException
from gslib.cloud_api import NotFoundException
from gslib.command import Command
from gslib.command_argument import CommandArgument
from gslib.cs_api_map import ApiSelector
from gslib.exception import CommandException
from gslib.exception import InvalidUrlError
from gslib.storage_url import ContainsWildcard
from gslib.storage_url import StorageUrlFromString
from gslib.util import NO_MAX
from gslib.util import PrintFullInfoAboutObject


_SYNOPSIS = """
  gsutil stat url...
"""

_DETAILED_HELP_TEXT = ("""
<B>SYNOPSIS</B>
""" + _SYNOPSIS + """


<B>DESCRIPTION</B>
  The stat command will output details about the specified object URLs.
  It is similar to running:

    gsutil ls -L gs://some-bucket/some-object

  but is more efficient because it avoids performing bucket listings and gets
  the minimum necessary amount of object metadata. Moreover, because it avoids
  performing bucket listings (which are eventually consistent) the gsutil stat
  command provides a strongly consistent way to check for the existence (and
  read the metadata) of an object.

  The gsutil stat command will, however, perform bucket listings if you specify
  URLs using wildcards.

  If run with the gsutil -q option nothing will be printed, e.g.:

    gsutil -q stat gs://some-bucket/some-object

  This can be useful for writing scripts, because the exit status will be 0 for
  an existing object and 1 for a non-existent object.

  Note: Unlike the gsutil ls command, the stat command does not support
  operations on sub-directories. For example, if you run the command:

    gsutil -q stat gs://some-bucket/some-subdir/

  gsutil will look for information about an object called "some-subdir/" (with a
  trailing slash) inside the bucket "some-bucket", as opposed to operating on
  objects nested under gs://some-bucket/some-subdir/. Unless you actually have
  an object with that name, the operation will fail. However, you can use the
  stat command on objects within subdirectories. For example, this command will
  work as expected:

    gsutil -q stat gs://some-bucket/some-subdir/file.txt
""")


# TODO: Add ability to stat buckets.
class StatCommand(Command):
  """Implementation of gsutil stat command."""

  # Command specification. See base class for documentation.
  command_spec = Command.CreateCommandSpec(
      'stat',
      command_name_aliases=[],
      usage_synopsis=_SYNOPSIS,
      min_args=1,
      max_args=NO_MAX,
      supported_sub_args='',
      file_url_ok=False,
      provider_url_ok=False,
      urls_start_arg=0,
      gs_api_support=[ApiSelector.XML, ApiSelector.JSON],
      gs_default_api=ApiSelector.JSON,
      argparse_arguments=[
          CommandArgument.MakeZeroOrMoreCloudURLsArgument()
      ]
  )
  # Help specification. See help_provider.py for documentation.
  help_spec = Command.HelpSpec(
      help_name='stat',
      help_name_aliases=[],
      help_type='command_help',
      help_one_line_summary='Display object status',
      help_text=_DETAILED_HELP_TEXT,
      subcommand_help_text={},
  )

  def RunCommand(self):
    """Command entry point for stat command."""
    # List of fields we'll print for stat objects.
    stat_fields = ['updated', 'cacheControl', 'contentDisposition',
                   'contentEncoding', 'contentLanguage', 'size', 'contentType',
                   'componentCount', 'metadata', 'crc32c', 'md5Hash', 'etag',
                   'generation', 'metageneration']
    found_nonmatching_arg = False
    for url_str in self.args:
      arg_matches = 0
      url = StorageUrlFromString(url_str)
      if not url.IsObject():
        raise CommandException('The stat command only works with object URLs')
      try:
        if ContainsWildcard(url_str):
          blr_iter = self.WildcardIterator(url_str).IterObjects(
              bucket_listing_fields=stat_fields)
        else:
          single_obj = self.gsutil_api.GetObjectMetadata(
              url.bucket_name, url.object_name, generation=url.generation,
              provider=url.scheme, fields=stat_fields)
          blr_iter = [BucketListingObject(url, root_object=single_obj)]
        for blr in blr_iter:
          if blr.IsObject():
            arg_matches += 1
            if logging.getLogger().isEnabledFor(logging.INFO):
              PrintFullInfoAboutObject(blr, incl_acl=False)
      except AccessDeniedException:
        logging.info('You aren\'t authorized to read %s - skipping', url_str)
      except InvalidUrlError:
        raise
      except NotFoundException:
        pass
      if not arg_matches:
        if logging.getLogger().isEnabledFor(logging.INFO):
          logging.info('No URLs matched %s', url_str)
        found_nonmatching_arg = True
    if found_nonmatching_arg:
      return 1
    return 0

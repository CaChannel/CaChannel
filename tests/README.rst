Test Suits
==========

1. Load ``test.db``::

  $ softIoc -d test.db

2. Test ``CaChannel.py`` with doctest::

  $ python -m CaChannel.CaChannel -v

3. Other tests from original caPython/CaChannel.

  - ca_wf.py: waveform handling
  - ca_gr_cb.py: callbacks

4. Test the ``ca`` module::

  $ python ca_test.py
